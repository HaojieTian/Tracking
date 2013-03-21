#include "Identification.h"


namespace Identification
{
	
	//////////     Module     ///////////
	/////////////////////////////////////

	void Identifier::init(Algorithm algorithmName)
	{
		switch(algorithmName)
		{
		case Naive:
			algorithm = &Identifier::algorithm_naive;
			break;
		case Test:
			algorithm = &Identifier::algorithm2;
			break;
		case Experimental:
			algorithm = &Identifier::algorithm3;
			break;
		}
	}

	void Identifier::identify(std::list<Frame> & frames)
	{
		if(frames.size() == 1)	// First time no objects are identified
		{
			for(int i = 0; i < frames.front().objects.size(); i++)
			{
				frames.front().objects[i].id = newID();
			}
		}
		else
		{
			(this->*algorithm)(frames);
		}
	}

	void Identifier::algorithm_naive(std::list<Frame> & frames)
	{
		Frame * current = &frames.front();
		Frame * previous = &(*(++frames.begin()));
		
		// Find the previous object which is probably the current object
		float distanceError, error;
		int pIndex, prevpIndex;
		std::list<ProbabilityContainer> mostProbable;

		isDecided.clear();
		for(std::vector<Object>::iterator p = previous->objects.begin(); p != previous->objects.end(); p++)
			isDecided.push_back(false);

		for(std::vector<Object>::iterator c = current->objects.begin(); c != current->objects.end(); c++)
		{
			mostProbable.push_back(ProbabilityContainer(-1, -1, 1000000));
			pIndex = 0;
			prevpIndex = -1;
			for(std::vector<Object>::iterator p = previous->objects.begin(); p != previous->objects.end(); p++)
			{
				/*
				 * distanceError = (x-x0-vx0)^2 + (y-y0-vy0)^2
				 */
				distanceError = std::pow(c->x - p->x - p->dx, 2) + std::pow(c->y - p->y - p->dy, 2);

				error = distanceError;
				if(!isDecided[pIndex] && mostProbable.back().error > error && error < 5000)
				{
					mostProbable.back().error = error;
					mostProbable.back().index = pIndex;
					if(prevpIndex >= 0)
						isDecided[prevpIndex] = false;
					isDecided[pIndex] = true;
					prevpIndex = pIndex;
				}
				pIndex++;
			}
		}

		pIndex = 0;
		for(std::list<ProbabilityContainer>::iterator p = mostProbable.begin(); p != mostProbable.end(); p++)
		{
			if(p->index >= 0)
				current->objects[pIndex].id = previous->objects[p->index].id;
			else
				current->objects[pIndex].id = newID();
			pIndex++;
		}
		
	}

	void Identifier::algorithm2(std::list<Frame> & frames)
	{
		
		Frame * current = &frames.front();
		Frame * previous = &(*(++frames.begin()));

		mostProbable.clear();
		undecidedObjects.clear();
		float distanceError, error;
		
		for(int i = 0; i < current->objects.size(); i++)
		{
			undecidedObjects.push_back(i);
			mostProbable.push_back(std::list<ProbabilityContainer>());
			
			for(int j = 0; j < previous->objects.size(); j++)
			{
				/*
				 * distanceError = (x-x0-vx0)^2 + (y-y0-vy0)^2
				 */
				distanceError = std::pow(current->objects[i].x - previous->objects[j].x - previous->objects[j].dx, 2) + std::pow(current->objects[i].y - previous->objects[j].y - previous->objects[j].dy, 2);
				error = distanceError;
				
				mostProbable[i].push_back(ProbabilityContainer(j,previous->objects[j].id,error));
			}
			mostProbable[i].sort();
		}

		//std::cout << "\nFind most probable previous object:\n";
		std::list<int>::iterator bestMatch;
		int matchingPrevious;
		float min;
		for(int candidate = 0; candidate < std::min(previous->objects.size(), current->objects.size()); candidate++)
		{
			min = 1000000;
			for(std::list<int>::iterator i = undecidedObjects.begin(); i != undecidedObjects.end(); i++)
			{
				if(mostProbable[*i].front().error < min)
				{
					min = mostProbable[*i].front().error;
					bestMatch = i;
				}
			}

			//A most probable candidate found!
			matchingPrevious = mostProbable[*bestMatch].front().index;
			current->objects[*bestMatch].id = previous->objects[matchingPrevious].id;

			//std::cout << "\tObject " << mostProbable[*bestMatch].front().probableId << " found with minError " << mostProbable[*bestMatch].front().error << "\n";

			for(std::list<int>::iterator i = undecidedObjects.begin(); i != undecidedObjects.end(); i++)
			{
				std::list<ProbabilityContainer>::iterator it = mostProbable[*i].begin();
				while(it != mostProbable[*i].end())
				{
					if(it->index == matchingPrevious)
						mostProbable[*i].erase(it++);
					else
						it++;
				}
				mostProbable[*i].sort();
			}
			undecidedObjects.erase(bestMatch);
		}

		
		//Take care of the new objects detected (still undecided):
		for(std::list<int>::iterator i = undecidedObjects.begin(); i != undecidedObjects.end(); i++)
		{
			current->objects[*i].id = newID();
		}

	}

	void Identifier::algorithm3(std::list<Frame> & frames)
	{
		/*
		// Select the objects in the previous frame that are candidates to the unidentified in the current frame
		std::vector<Object *> candidates;
		candidates.clear();
		for(std::vector<Object>::iterator c = previous->objects.begin(); c != previous->objects.end(); c++)
		{
			if(!probablyOutsideOfImage(*c))
				candidates.push_back(&(*c));
		}
		*/
	}



	////////// TEST GENERATION ///////////
	//////////////////////////////////////

	const int cTEST_FRAME_WIDTH = 480;
	const int cTEST_FRAME_HEIGHT = 360;

	#define NEW_FRAME() frameList.push_back(Frame(cv::Mat(cTEST_FRAME_HEIGHT, cTEST_FRAME_WIDTH, CV_8UC3), cv::Mat(cTEST_FRAME_HEIGHT, cTEST_FRAME_WIDTH, CV_8UC3))); frameList.back().image = Scalar(0,0,0);
	#define INSERT_OBJECT(x,y,dx,dy) frameList.back().objects.push_back(Object(x, y, dx, dy, 20, 60));
	//#define INSERT_OBJECT(x,y) frameList.back().objects.push_back(Object(x, y, 0, 0, 20, 60));

	void generate_testdata(std::list<Frame> & frameList, std::string test)
	{
		if(test == "simple1")
		{
			int stepLength = 10;
			float var = 10;	// Variance
			for(int i = 0; i < cTEST_FRAME_WIDTH; i+=stepLength)
			{
				NEW_FRAME();
				INSERT_OBJECT(i, 200+var*randf(), stepLength+var*randf(), var*randf());
				INSERT_OBJECT(i, 100+10*randf(), stepLength+var*randf(), var*randf());
			}
		}
		else if(test == "complex1")
		{
			float var = 20;	// Variance
			for(int t = 0; t < cTEST_FRAME_WIDTH; t++)
			{
				NEW_FRAME();
				INSERT_OBJECT(t*10, 200+var*randf(), 10+var*randf(), var*randf());
				INSERT_OBJECT(20+t*10+10*randf(), 20+t*10+10*randf(), 10+var*randf(), var*randf());
			}
		}

	}

	float randf()
	{
		return (float(rand()) - float(RAND_MAX)/2.0)/RAND_MAX;
	}

};