#pragma once
#define PI 3.14159

class grid
{
private:
	static const int NUM_CELLS = 10;
	int CELL_SIZE = 1;
	int simSize;
	float smoothingRadius;
	int boundX;
	int boundY;

	float reflectionEff;
	int particleSize;

	float targetDensity;
	float pressureMultiplier;

	float friction;

public:
	particle* cells[NUM_CELLS][NUM_CELLS];

	// Constructor
	grid(int size, float sR, int bX, int bY, float _refEff, int _particleSize, float _targetDensity, float _pressureMultiplier, float _friction): friction(_friction),
		simSize(size),
		smoothingRadius(sR),
		boundX(bX),
		boundY(bY),
		reflectionEff(_refEff),
		particleSize(_particleSize),
		targetDensity(_targetDensity),
		pressureMultiplier(_pressureMultiplier)
	
	
	{
		CELL_SIZE = bX / NUM_CELLS;


		for (int y = 0; y < NUM_CELLS; y++)
		{
			for (int x = 0; x < NUM_CELLS; x++)
			{
				cells[y][x] = NULL;
			}
		}
	}

	// Getters
	int numCells() { return NUM_CELLS*NUM_CELLS; }
	int cellSize() { return CELL_SIZE; }
	float TargetDensity() { return targetDensity; }

	void TargetDensity(float _density) { targetDensity = _density; }

	void add(particle* p) // Properly sorts particle into it's appropriate cell and adds it to the linked list
	{
		
		// Finds which cell the particle should go into
		
		int cellX = static_cast<int>(p->pos.x + (boundX / 2)) / CELL_SIZE;
		int cellY = static_cast<int>(p->pos.y + (boundY / 2)) / CELL_SIZE;


		// Sets the pointers for the linked list
		p->prev = NULL;
		p->next = cells[cellY][cellX];
		cells[cellY][cellX] = p;

		if (p->next != NULL)
		{
			p->next->prev = p;
		}
	}

	void remove(particle* p) // Removes particle from the grid and deallocates the memory
	{
		int cellX = static_cast<int>(p->pos.x + (boundX / 2)) / CELL_SIZE;
		int cellY = static_cast<int>(p->pos.y + (boundY / 2)) / CELL_SIZE;


		// Stitches the two adjacent particles in the list together in effect removing the particle from the list
		if (p->prev != NULL)
		{
			p->prev->next = p->next;
		}
		if (p->next != NULL)
		{
			p->next->prev = p->prev;
		}

		// If particle was the head of the list make the next particle the head
		if (cells[cellY][cellX] == p)
		{
			cells[cellY][cellX] = p->next;
		}

		delete p; // and finally deallocate the particle to prevent memory leaks
	}

	void moveParticle(particle* p, float x, float y)
	{
		int cellX = static_cast<int>(p->pos.x + (boundX / 2)) / CELL_SIZE;
		int cellY = static_cast<int>(p->pos.y + (boundY / 2)) / CELL_SIZE;
		
		p->pos.x = x;
		p->pos.y = y;

		ResolveCollisions(p);

		int newCellX = static_cast<int>(p->pos.x + (boundX / 2)) / CELL_SIZE;
		int newCellY = static_cast<int>(p->pos.y + (boundY / 2)) / CELL_SIZE;
		

		// If particle has moved over grid boundary then we execute everything after this if statement
		if (((newCellX == cellX) && (newCellY == cellY)))
		{
			return;
		}

		// Remove particle from linked list
		if (p->prev != NULL)
		{
			p->prev->next = p->next;
		}
		if (p->next != NULL)
		{
			p->next->prev = p->prev;
		}

		// If particle was the head of the list make the next particle the head
		if (cells[cellY][cellX] == p)
		{
			cells[cellY][cellX] = p->next;
		}

		// Add particle into new cell
		add(p);
	}

	void ResolveCollisions(particle* p)
	{
		int xBound = (boundX - particleSize) / 2;
		int yBound = (boundY - particleSize) / 2;
		int x = 0;
		int y = 0;

		if (std::abs(p->pos.x) > xBound)
		{
			p->pos.x = (xBound) * ((p->pos.x > 0) - (p->pos.x < 0));
			p->vel.x *= -1 * reflectionEff;
		}

		if (std::abs(p->pos.y) > yBound)
		{
			p->pos.y = (yBound) * ((p->pos.y > 0) - (p->pos.y < 0));
			p->vel.y *= -1 * reflectionEff;
		}


	}

	// Pressure Functions
	float DensityToPressure(float density)
	{
		float densityError = density - targetDensity;
		float pressure = densityError * pressureMultiplier;
		return pressure;
	}

	float SharedPressure(particle* A, particle* B)
	{
		float AP = DensityToPressure(A->density);
		float BP = DensityToPressure(B->density);

		return (AP + BP) / 2.0f;
	}


	// Smoothing Kernals
	float Smoothing(float radius, float dst)
	{
		radius *= 0.001;
		dst *= 0.001;
		if (dst >= radius) return 0;

		float volume = PI * pow(radius, 4) / 6;
		return (radius - dst) * (radius - dst) / volume;
	}

	float SmoothingDerivative(float radius, float dst)
	{
		radius *= 0.001;
		dst *= 0.001;
		if (dst >= radius) return 0;
		float scale = 12 / (pow(radius, 4) * PI);
		return (dst - radius) * scale;
	}

	float CalculateDensity(vec2d samplePoint)
	{
		float density = 0.0f;
		double dst = 0.0f;
		float influence = 0.0f;

		int cellX = static_cast<int>(samplePoint.x + (boundX / 2)) / CELL_SIZE;
		int cellY = static_cast<int>(samplePoint.y + (boundY / 2)) / CELL_SIZE;

		// Loop over all particle positions
		for (int row = -1; row < 2; row++)
		{
			for (int col = -1; col < 2; col++)
			{
				int coX = cellX + row;
				int coY = cellY + col;
				if ((coX > -1) && (coX < 10) && (coY > -1) && (coY < 10))
				{

					particle* p = cells[coY][coX];
					while (p != NULL)
					{
						dst = dist(p->prePos, samplePoint);
						influence = Smoothing(smoothingRadius, dst);
						density += p->mass * influence;

						p = p->next;
					}
				}
			}
		}
		return density;
	}

	// Calculate the direction of movement for the particles
	vec2d CalculateGradient(vec2d samplePoint, particle* _p)
	{
		int cellX = static_cast<int>(samplePoint.x + (boundX / 2)) / CELL_SIZE;
		int cellY = static_cast<int>(samplePoint.y + (boundY / 2)) / CELL_SIZE;

		float density = 0.0f;
		float sharedPressure = 0.0f;
		double dst = 0.0f;
		float slope = 0.0f;

		vec2d propertyGradient;
		propertyGradient.x = 0;
		propertyGradient.y = 0;

		vec2d dir;
		dir.x = 0;
		dir.y = 0;

		vec2d avgVel;
		avgVel.x = 0;
		avgVel.y = 0;

		int particleCount = 0;


		// Loop over all particle positions in surrounding cells
		for (int row = -1; row < 2; row++)
		{
			for (int col = -1; col < 2; col++)
			{
				int coX = cellX + row;
				int coY = cellY + col;
				if ((coX > -1) && (coX < 10) && (coY > -1) && (coY < 10)) // Making sure we only consider cells that are within bounds
				{

					particle* p = cells[coY][coX];

					while (p != NULL)
					{
						if (p != _p)
						{
							dst = dist(p->pos, samplePoint);

							// Prevent Divide by zero error if two particles are on top of each other
							if (dst != 0)
							{
								dir.x = (p->pos.x - samplePoint.x) / dst;
								dir.y = (p->pos.y - samplePoint.y) / dst;
							}
							else
							{
								dir.x = (-100 + rand() % 101);
								dir.y = (-100 + rand() % 101);

								float magnitude = sqrt(dir.x * dir.x + dir.y * dir.y);
								
								dir.x /= magnitude;
								dir.y /= magnitude;
							}



							slope = SmoothingDerivative(smoothingRadius, dst);
							density = p->density;
							sharedPressure = SharedPressure(p, _p);

							propertyGradient.x += sharedPressure * p->mass * slope * dir.x / density;
							propertyGradient.y += sharedPressure * p->mass * slope * dir.y / density;

							avgVel.x += p->vel.x * slope;
							avgVel.y += p->vel.y * slope;
							particleCount++;
						}

						p = p->next;
					}
				}
			}
		}
		avgVel.x /= particleCount;
		avgVel.y /= particleCount;

		vec2d deltaVel;
		deltaVel.x = avgVel.x - _p->vel.x;
		deltaVel.y = avgVel.y - _p->vel.y;

		propertyGradient.x += deltaVel.x * friction;
		propertyGradient.y += deltaVel.y * friction;

		return propertyGradient;
	}
};
