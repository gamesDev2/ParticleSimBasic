#include <iostream>
#include <chrono>
#include <cmath>
#include "particle.h"
#include "grid.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.14159


class Screen : public olc::PixelGameEngine
{
	
public:

	// Simulation parameters
	int simSize;
	float simSpeed = 1;

	float gravity = 1000;
	float reflectionEff = 0.2;

	float targetDensity = 1000;
	float pressureMultiplier = 10000;
	float friction = 1;

	int particleSize = 4;
	float particleSpacing = 10;

	float smoothingRadius = 90;

	int boundaryX = 900;
	int boundaryY = boundaryX;
	
	// Simulation Space
	grid* area;

	// A pointer to a particle that we want to track
	particle* markedParticle;


	// Program control variables
	bool paused = true;
	bool drawPartitions = true;


public:

	Screen(int _simSize)
	{
		sAppName = "Particles";
		simSize = _simSize;
	}

public:

	bool OnUserCreate() override
	{
		// Initializing Simulation space
		area = new grid(simSize, smoothingRadius, boundaryX, boundaryY, reflectionEff, particleSize, targetDensity, pressureMultiplier, friction);
		


		particle* particlePtr;

		int particlesPerRow = std::sqrt(simSize);
		int particlesPerCol = (simSize - 1) / particlesPerRow + 1;
		float spacing = particleSize * 2 + particleSpacing;
		float x;
		float y;
		// Creating all of the particles in a grid pattern and adding them to the simulation space
		for (int i = 0; i < simSize; i++)
		{

			x = (i % particlesPerRow - (particlesPerRow / 2) + 0.5) * spacing;
			y = (i / particlesPerRow - (particlesPerCol / 2) + 0.5) * spacing;

			//x = -(boundaryX / 2) + rand() % boundaryX;
			//y = -(boundaryY / 2) + rand() % boundaryY;

			particlePtr = new particle;

			// Iniializing particle values as particle is a struct not a class so it has no constructor of it's own
			particlePtr->pos.x = x;
			particlePtr->pos.y = y;

			particlePtr->prePos.x = x;
			particlePtr->prePos.y = y;

			particlePtr->vel.x = 0;
			particlePtr->vel.y = 0;

			particlePtr->mass = 1;
			particlePtr->charge = 1;


			area->add(particlePtr);
			if (i == simSize/2)
			{
				markedParticle = particlePtr;
			}
		}

		Clear(olc::BLACK);

		drawActiveCells();
			

		updateParticles(0); 
			
		//DrawCircle(GetMousePos(), smoothingRadius, olc::BLUE);
		
		drawBoundaries(); 
		updateControl(0); 
		return true;
	}


	// called once per frame
	bool OnUserUpdate(float fElapsedTime) override
	{
		if (!paused)
		{
			Clear(olc::BLACK);

			if (drawPartitions)
			{
				drawActiveCells();
			}

			updateParticles(fElapsedTime);
			drawBoundaries();

		}


		updateControl(fElapsedTime);
		
		

		return true;
	}


	void updateParticles(float fElapsedTime)
	{
		fElapsedTime *= simSpeed; // We use elapsed time to make sure everything is synced up correctly so we can use this fact to change how quickly the simulation runs by multiplying it by the simSpeed

		int x = 0;
		int y = 0;


		int rowLength = sqrt(area->numCells());
		int gridLength = area->numCells();

		for (int cell = 0; cell < gridLength; cell++) // Loop for density
		{
			int cellY = cell / rowLength;
			int cellX = cell % rowLength;

			particle* p = area->cells[cellY][cellX];
			particle* nextP;
			while (p != NULL)
			{
				p->density = area->CalculateDensity(p->prePos);

				p = p->next;
			}
		}

		// Looping Through the particles for final
		for (int cell = 0; cell < gridLength; cell++)
		{
			int cellY = cell / rowLength;
			int cellX = cell % rowLength;

			particle* p = area->cells[cellY][cellX];
			particle* nextP;
			while (p != NULL)
			{
				
				nextP = p->next; // We do this first to make sure particles are not skipped over when previous particles move over grid boundaries

				// Gravity
				p->vel.y += (gravity) * fElapsedTime;


				vec2d gradient;
				vec2d accelleration;
				// Finding the force that is applied to each particle
				gradient = area->CalculateGradient(p->prePos, p);
				// Finding how fast the particle accelerates due to that force
				accelleration.x = gradient.x / p->density;
				accelleration.y = gradient.y / p->density;

				// Changing velocity accordingly
				p->vel.x += accelleration.x * fElapsedTime;
				p->vel.y += accelleration.y * fElapsedTime;

				
				float newX = p->pos.x + (p->vel.x * fElapsedTime);
				float newY = p->pos.y + (p->vel.y * fElapsedTime);

				if (isnan(newX) || isnan(newY))
				{
					newX = p->prePos.x + (-10 + rand() % 20);
					newY = p->prePos.y + (-10 + rand() % 20);

					p->vel.x = 0;
					p->vel.y = 0;
				}
				// Moving particles to there new positions
				area->moveParticle(p, newX, newY);

				p->prePos.x = p->pos.x + p->vel.x * fElapsedTime;
				p->prePos.y = p->pos.y + p->vel.y * fElapsedTime;

				// Putting the positions into global space for rendering
				x = p->pos.x + ScreenWidth() / 2;
				y = p->pos.y + ScreenHeight() / 2;

				FillCircle(x, y , particleSize, olc::WHITE);

				p = nextP;

			}
		}
	}

	void drawBoundaries()
	{
		int lX = (-boundaryX / 2) + ScreenWidth() / 2;
		int tY = (-boundaryY / 2) + ScreenHeight() / 2;

		int rX = (boundaryX / 2) + ScreenWidth() / 2;
		int bY = (boundaryY / 2) + ScreenHeight() / 2;

		DrawLine(lX, tY, rX, tY, olc::YELLOW);
		DrawLine(lX, tY, lX, bY, olc::YELLOW);
		DrawLine(lX, bY, rX, bY, olc::YELLOW);
		DrawLine(rX, bY, rX, tY, olc::YELLOW);
	}

	void drawActiveCells()
	{
		int cellSize = area->cellSize();
		int gridLength = area->numCells();
		int rowLength = sqrt(area->numCells());
		int x = 0;
		int y = 0;

		for (int cell = 0; cell < gridLength; cell++)
		{
			int cellY = cell / rowLength;
			int cellX = cell % rowLength;

			if (area->cells[cellY][cellX] != NULL)
			{
				x = (boundaryX * (cellX / 10.0f) + (-boundaryX / 2) + ScreenWidth() / 2);
				y = (boundaryY * (cellY / 10.0f) + (-boundaryY / 2) + ScreenHeight() / 2);
				drawActiveCell(x, y, cellSize, cell);
			}

		}
	}

	void drawActiveCell(int _x, int _y, int _cellSize, int _cell)
	{
		std::string cellNumber = std::to_string(_cell);;
		DrawRect(_x, _y, _cellSize, _cellSize, olc::YELLOW);
		DrawString(_x + (_cellSize / 2), _y + (_cellSize / 2), cellNumber, olc::YELLOW);

	}

	void updateControl(float fElapsedTime)
	{
		if (GetKey(olc::Key::SPACE).bPressed)
		{
			paused = !paused;
		}

		if (GetKey(olc::Key::P).bPressed)
		{
			drawPartitions = !drawPartitions;
		}

		if (GetKey(olc::Key::D).bPressed)
		{
			std::cout << markedParticle->density << std::endl;
		}

		if (GetKey(olc::Key::UP).bPressed)
		{
			area->TargetDensity(area->TargetDensity() + 100);
			std::cout << area->TargetDensity() << std::endl;
			
		}

		if (GetKey(olc::Key::DOWN).bPressed)
		{
			area->TargetDensity(area->TargetDensity() - 100);
			std::cout << area->TargetDensity() << std::endl;
		}

		if (GetMouse(0).bHeld)
		{
			particle* newP = new particle;

			newP->pos.x = GetMouseX() - ScreenWidth() / 2;
			newP->pos.y = GetMouseY() - ScreenHeight() / 2;
			newP->prePos = newP->pos;

			newP->vel.x = -50 + rand() % 101;
			newP->vel.y = 0;

			newP->charge = 1;
			newP->mass = 1;
			newP->density = 1;

			area->add(newP);
		}

	}
};

int main()
{
	Screen sim(700);
	if (sim.Construct(1000, 1000, 1, 1))
		sim.Start();

	return 0;
}