#include "DSA_controller.h"

/*****
 * Initialize most basic variables and objects here. Most of the setup should
 * be done in the Init(...) function instead of here where possible.
 *****/
DSA_controller::DSA_controller() :
    NumberOfRobots(0),
    NumberOfSpirals(0),
    DSA(RETURN_TO_NEST),
    RNG(NULL),
    ResetReturnPosition(true),
    stopTimeStep(0),
    isHoldingFood(false),
    StepsToActivateAlgorithm(0)
{}

/*****
 * Initialize the controller via the XML configuration file. ARGoS typically
 * wants objects & variables initialized here instead of in the constructor(s).
 *****/
void DSA_controller::Init(TConfigurationNode& node) {

    compassSensor   = GetSensor<argos::CCI_PositioningSensor>("positioning");
    wheelActuator   = GetActuator<argos::CCI_DifferentialSteeringActuator>("differential_steering");
    proximitySensor = GetSensor<argos::CCI_FootBotProximitySensor>("footbot_proximity");

    argos::TConfigurationNode settings = argos::GetNode(node, "settings");
    argos::GetNodeAttribute(settings, "NumberOfRobots",          NumberOfRobots);
    argos::GetNodeAttribute(settings, "NumberOfSpirals",         NumberOfSpirals);
    argos::GetNodeAttribute(settings, "SearchStepSize",          SearchStepSize);
    argos::GetNodeAttribute(settings, "NestDistanceTolerance", NestDistanceTolerance);
    argos::GetNodeAttribute(settings, "NestAngleTolerance", NestAngleTolerance);
    argos::GetNodeAttribute(settings, "NestAngleTolerance", NestAngleTolerance);
    argos::GetNodeAttribute(settings, "TargetDistanceTolerance", TargetDistanceTolerance);
    argos::GetNodeAttribute(settings, "TargetAngleTolerance",    TargetAngleTolerance);
    argos::GetNodeAttribute(settings, "SearcherGap",             SearcherGap);
    argos::GetNodeAttribute(settings, "FoodDistanceTolerance",   FoodDistanceTolerance);
    argos::GetNodeAttribute(settings, "RobotForwardSpeed",       RobotForwardSpeed);
    argos::GetNodeAttribute(settings, "RobotRotationSpeed",      RobotRotationSpeed);
    argos::GetNodeAttribute(settings, "ResultsDirectoryPath",      results_path);
    argos::GetNodeAttribute(settings, "DestinationNoiseStdev",      DestinationNoiseStdev);
    argos::GetNodeAttribute(settings, "PositionNoiseStdev",      PositionNoiseStdev);
    argos::GetNodeAttribute(settings, "ProbTargetDetection",      ProbTargetDetection);
    FoodDistanceTolerance *= FoodDistanceTolerance;

    argos::CVector2 p(GetPosition());
    SetStartPosition(argos::CVector3(p.GetX(), p.GetY(), 0.0));
    
    //startPosition = CVector3(0.0, 0.0, 0.0);
    startPosition    = CVector3(p.GetX(), p.GetY(), 0.0);
    
    RNG = CRandom::CreateRNG("argos");
    generatePattern(NumberOfSpirals, NumberOfRobots);
    TrailColor = CColor(std::rand()%255, std::rand()%255, std::rand()%255, 255);

    // Name the results file with the current time and date
 time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    stringstream ss;

    ss << "DSA-"<<GIT_BRANCH<<"-"<<GIT_COMMIT_HASH<<"-" 
       << (now->tm_year) << '-'
       << (now->tm_mon + 1) << '-'
       <<  now->tm_mday << '-'
       <<  now->tm_hour << '-'
       <<  now->tm_min << '-'
       <<  now->tm_sec << ".csv";

    string results_file_name = ss.str();
   results_full_path = results_path+"/"+results_file_name;        



    // Only the first robot should do this:


    if (GetId().compare("DSA_0") == 0)
      {
  
   ofstream results_output_stream;
 results_output_stream.open(results_full_path, ios::app);
 results_output_stream << "NumberOfRobots, "
		       << "NumberOfSpirals, "
		       << "TargetDistanceTolerance, "
		       << "TargetAngleTolerance, "
		       << "SearcherGap, "
		       << "FoodDistanceTolerance, "
		       << "RobotForwardSpeed, "
		       << "RobotRotationSpeed, "
		       << "RandomSeed" << endl
               << NumberOfRobots << ", "
		       << NumberOfSpirals << ", "
		       << TargetDistanceTolerance << ", "
		       << TargetAngleTolerance << ", "
		       << SearcherGap << ", "
		       << FoodDistanceTolerance << ", "
		       << RobotForwardSpeed << ", "
		       << RobotRotationSpeed << ", "
		       << CSimulator::GetInstance().GetRandomSeed() << endl;  
 results_output_stream.close();
      }

    std::string id = GetId();
    std::string extracted_str = extractID(id);
    if(id.length() > 0)
    {
        stRobotData.id_robot = std::stoi(extracted_str);
    }
    
    FirstTimeSearch = 0;
//    ResumeMovemnet = 0;
    cout << "Finished Initializing the DDSA" << endl;
}

/************************************************/
/* Function to get the number in the string */
/************************************************/
std::string DSA_controller::extractID(std::string str)
{
    argos::UInt8 counter;
    std::string output_str;
    for (counter=0; counter < str.length(); counter++){
        if (isdigit(str[counter])){
            output_str+= str[counter];
        }
    }
    return output_str;
}

size_t DSA_controller::generatePattern(int N_circuits, int N_robots)
{
    string ID = GetId();
    string ID_number;

    for(size_t i = 0; i < ID.size(); i++) {
        if(ID[i] >= '0' && ID[i] <= '9') {
            ID_number += ID[i];
        }
    }

    size_t RobotNumber = stoi(ID_number);
    vector<string> paths;
    string ith_robot_path;

    for (int i_robot = 1; i_robot <= N_robots; i_robot++)
    {
        // cout << "inside for 1" << endl;
        for (int i_circuit = 0; i_circuit < N_circuits; i_circuit++)
        {
            int n_steps_north = calcDistanceToTravel(i_robot, i_circuit, N_robots, 'N');
            for (int j = 0; j < n_steps_north; j++)
            {
                //ith_robot_path.push_back('N');
                ith_robot_path += 'N';
                if((i_robot-1) == RobotNumber && i_circuit < 1)
                {
                    StepsToActivateAlgorithm++;
                }
            }
            
            int n_steps_east = calcDistanceToTravel(i_robot, i_circuit, N_robots, 'E');
            for (int j = 0; j < n_steps_east; j++)
            {
                //ith_robot_path.push_back('E');
                ith_robot_path += 'E';
                if(i_robot == RobotNumber && i_circuit < 1)
                {
                    StepsToActivateAlgorithm++;
                }
            }

            int n_steps_south = calcDistanceToTravel(i_robot, i_circuit, N_robots, 'S');
            for (int j = 0; j < n_steps_south; j++)
            {
                //ith_robot_path.push_back('S');
                ith_robot_path += 'S';
                if((i_robot - 1) == RobotNumber && i_circuit < 1)
                {
                    StepsToActivateAlgorithm++;
                }
            }

            int n_steps_west = calcDistanceToTravel(i_robot, i_circuit, N_robots, 'W');
            for (int j = 0; j < n_steps_west; j++)
            {
                //ith_robot_path.push_back('W');
                ith_robot_path += 'W';
                if((i_robot-1) == RobotNumber && i_circuit < 1)
                {
                    StepsToActivateAlgorithm++;
                }
            }

        }

        paths.push_back(ith_robot_path);
        ith_robot_path.clear();
        StepsToActivateAlgorithm = 0;
    }

    //pattern = ith_robot_path;
    GetPattern(paths[RobotNumber]);

    return RobotNumber;
}

int DSA_controller::calcDistanceToTravel(int i_robot, int i_circuit, int N_robots, char direction)
{
    int i = i_robot;
    int j = i_circuit;
    int N = N_robots;
    int n_steps  = 0;

    if (direction == 'N' || direction == 'E')
    {
        if (j == 0)
        {
            n_steps = i;
            return n_steps;
        }
        else if (j == 1)
        {
            n_steps = calcDistanceToTravel(i, j-1, N, direction) + i + N;
            return n_steps;
        }
        else 
        {
            n_steps = calcDistanceToTravel(i, j-1, N, direction) + 2*N;
            return n_steps;
        }
    }

    else if (direction == 'S' || direction == 'W')
    {
        if (j == 0)
        {
            n_steps = calcDistanceToTravel(i, j , N, 'N') + i;
            return n_steps;
        }

        else if (j > 0)
        {
            n_steps = calcDistanceToTravel(i, j, N, 'N') + N;
            return n_steps;
        }

        else
        {
            cout << "Error direction" << direction << "is invalid" << endl;
        }

    }
    return 0;
}

void DSA_controller::printPath(vector<char>& path)
{
    cout << path.size() << endl;
    for(int i = 0; i<path.size(); i++)
    { 
        cout << path.at(i) << endl;
    }
}

void DSA_controller::GetPattern(string ith_Pattern)
{
    copy(ith_Pattern.begin(),ith_Pattern.end(),back_inserter(tempPattern));
    reverse(tempPattern.begin(), tempPattern.end());
    copy(tempPattern.begin(), tempPattern.end(),back_inserter(stRobotData.pattern));
    reverse(stRobotData.pattern.begin(), stRobotData.pattern.end());
}

// /*****
//  *
//  *****/
void DSA_controller::CopyPatterntoTemp() 
{
    copy(pattern.begin(),pattern.end(),back_inserter(tempPattern));
    reverse(tempPattern.begin(),tempPattern.end());/* Reverses the tempPattern */
    
}

/*****
 * Primary control loop for this controller object. This function will execute
 * the DSA logic once per frame.
 *****/
void DSA_controller::ControlStep() 
{
//    argos::LOG<<"Target collected: "<<num_targets_collected<<std::endl;
    argos::LOG<<"Robot ID: "<<stRobotData.id_robot<<std::endl;
//    argos::LOG<<"Robot ID: "<<RobotNumber<<std::endl;
    
//    argos::LOG<<"Distance: "<<stRobotData.Priority<<std::endl;
    argos::LOG<<"Ticks calculated for intersection: "<<stRobotData.Intial_TurningWaitTime<<std::endl;
    argos::LOG<<"Robot ID inter: "<<stRobotData.Priority<<std::endl;
//    argos::LOG<<"Safe distanace Ticks: "<<stRobotData.WaypointCounter<<std::endl;
//    argos::LOG<<"Simulator TicksPerSec: "<<loopFunctions->SimulatorTicksperSec<<std::endl;
    
//    argos::LOG<<"Holding food flag: "<< IsHoldingFood()<<std::endl;

//    argos::LOG<<"Return Position: "<<ReturnPosition<<std::endl;
    argos::LOG<<"DSA State: "<<DSA<<std::endl;
////    argos::LOG<<"movemnt state: "<<CurrentMovementState<<std::endl;
//    argos::LOG<<"Robot path Checked: "<<stRobotData.pathcheck<<std::endl;
//    argos::LOG<<"Steps To Activate Algorithm: "<<StepsToActivateAlgorithm<<std::endl;
    argos::LOG<<"First Check: "<<loopFunctions->FirstCheck<<std::endl;
    argos::LOG<<"Going to nest: "<<stRobotData.GoingToNest<<std::endl;
//     argos::LOG<<"Algorithm Checked: "<<stRobotData.Checked<<std::endl;
    
    argos::LOG<<"Going to/from nest: "<<stRobotData.GoingToOrFromNest<<std::endl;
//    argos::LOG<<"Point Change Direction: "<<loopFunctions->PointChangeDirection<<std::endl;
//    argos::LOG<<"Point Safe Distance: "<<loopFunctions->PointAtSafeDistance<<std::endl;
    
    
//    argos::LOG<<"Stack size: "<<stRobotData.WaypointStack.size()<<std::endl;
    argos::LOG<<"Start Location: "<<stRobotData.StartWaypoint<<std::endl;
//    argos::LOG<<"Current Location: "<<GetPosition()<<std::endl;
    argos::LOG<<"Target Location: "<<stRobotData.TargetWaypoint<<std::endl;
//
//    argos::LOG<<"Waypoint: "<<stRobotData.AddedPoint<<std::endl;
    argos::LOG<<"Movement State: "<<CurrentMovementState<<std::endl;
    argos::LOG<<"Movement Stack Size: "<<MovementStack.size()<<std::endl;
//    argos::LOG<<"Waypoint added: "<<stRobotData.Waypoint_Added<<std::endl;
//////    argos::LOG<<"Target reached: "<<stRobotData.WaypointReached<<std::endl;
    argos::LOG<<"Intersection flag: "<<st_IntersectionData.Intersection_flag<<std::endl;
    argos::LOG<<"Intersection point: "<<st_IntersectionData.IntersectionPoint<<std::endl;
//    argos::LOG<<"Collinear Flag: "<<stRobotData.CollinearFlag<<std::endl;
//    argos::LOG<<"Added Way Point: "<<stRobotData.AddedPoint<<std::endl;
//
//    argos::LOG<<"Robot Course Angle: "<<stRobotData.InitialOrientation<<std::endl;
//    argos::LOG<<"Robot Heading Angle: "<<stRobotData.HeadingAngle<<std::endl;
//    argos::LOG<<"Shortest Distance: "<<stRobotData.Priority<<std::endl;
    argos::LOG<<"Waypoint_Added Flag: "<<stRobotData.Waypoint_Added<<std::endl;
    argos::LOG<<"Waypoint Flag Loop Functions: "<<loopFunctions->NewWayPointAdded<<std::endl;
//    argos::LOG<<"Waypoint_Counter: "<<stRobotData.WaypointCounter<<std::endl;
//    argos::LOG<<"Linear Speed: "<<stRobotData.fLinearWheelSpeed<<std::endl;
    argos::LOG<<"Stop Turning Time: "<<stRobotData.StopTurningTime<<std::endl;
    argos::LOG<<"---------------------------------------------------------------------"<<std::endl;
    
    
    // To draw paths
    if (DSA == SEARCHING)
    {
        CVector3 position3d(GetPosition().GetX(), GetPosition().GetY(), 0.00);
        CVector3 target3d(previous_position.GetX(), previous_position.GetY(), 0.00);
        CRay3 targetRay(target3d, position3d);
        myTrail.push_back(targetRay);

        loopFunctions->TargetRayList.push_back(targetRay);
        loopFunctions->TargetRayColorList.push_back(TrailColor);
    }

  previous_position = GetPosition();
  
    
  /* Continue in a sprial */
  if( DSA == SEARCHING )
  {
      SetHoldingFood();
      SetIsHeadingToNest(false);
      stRobotData.GoingToNest = false;
      SetHoldingFood();
  
      if (IsHoldingFood())
      {
          bool cpf = true;
         
          if (cpf)
          {
//              SetHardStopMovement();
              ReturnPosition = GetPosition();
              ReturnPatternPosition = GetTarget();
              DSA = RETURN_TO_NEST;
              stRobotData.GoingToOrFromNest = true;
          }
          else
          {
              num_targets_collected++;
              loopFunctions->setScore(num_targets_collected);
              isHoldingFood = false;
          }
    
          return;
        }
      else
      {
  
//          if (loopFunctions->FirstCheck == 1 and IsAtTarget())
//          {
//              stRobotData.Waypoint_Added = false;
//          }
          if(FirstTimeSearch == StepsToActivateAlgorithm)
          {
              // set the flag to activate path planning algorithm
              stRobotData.Checked = true;
          }
          if(stRobotData.WaypointStack.empty())
          {
              GetTargets(); /* Initializes targets positions. */
          }
      
//          if(FirstTimeSearch < 3)
//          {
//              FirstTimeSearch++;
//          }

   
      }
      
      
    }
    
      else if( DSA == RETURN_TO_NEST)
      {
//        SetIsHeadingToNest(true);
//        stRobotData.GoingToNest = true;
        // Check if we reached the nest. If so record that we dropped food off and go back to the spiral
        if((GetPosition() - loopFunctions->NestPosition).SquareLength() < loopFunctions->NestRadiusSquared)
        {
            SetIsHeadingToNest(true);
            stRobotData.GoingToNest = true;
            if (isHoldingFood)
            {
                num_targets_collected++;
                loopFunctions->setScore(num_targets_collected);
            }
            
            isHoldingFood = false;
            
            
            SetIsHeadingToNest(false);
            stRobotData.GoingToNest = false;
            if(stRobotData.WaypointStack.empty())
            {
              DSA = RETURN_TO_SEARCH;
              stRobotData.Waypoint_Added = true;
              SetTarget(ReturnPosition);
              stRobotData.pathcheck = false;
              if(stRobotData.GoingToOrFromNest == true)
              {
                  SetHardStopMovement();
              }
            }
            
//          if(IsAtTarget())
//          {
//              DSA = RETURN_TO_SEARCH;
//              SetIsHeadingToNest(false);
//              stRobotData.GoingToNest = false;
//              if(stRobotData.WaypointStack.empty())
//              {
//                  stRobotData.Waypoint_Added = true;
//                  SetTarget(ReturnPosition);
//                  stRobotData.pathcheck = false;
//                  SetHardStopMovement();
//              }
//
//
//          }

          /*
          ofstream results_output_stream;
          results_output_stream.open(results_full_path, ios::app);
          results_output_stream << loopFunctions->getSimTimeInSeconds() << ", " << ++num_targets_collected << ", " << "Col Count" << endl;
          results_output_stream.close();
          */
        }
        else
        {
            // set the target if heading to nest is false i.e. entering this state
            //for the first time after collecting food
            if(stRobotData.WaypointStack.empty())
            {
                SetTarget(loopFunctions->NestPosition);
//            }
                if(stRobotData.GoingToNest == false)
                {
                    stRobotData.Waypoint_Added = true;
                    stRobotData.pathcheck = false;
//                    SetHardStopMovement();
                }
                SetIsHeadingToNest(true);
                stRobotData.GoingToNest = true;
            }
           
         
            
        }
      }
      else if( DSA == RETURN_TO_SEARCH )
      {
          SetIsHeadingToNest(false);
          stRobotData.GoingToNest = false;
      
          // Check if we have reached the return position
          if (IsAtTarget() and stRobotData.WaypointStack.empty())
          {
              stRobotData.Waypoint_Added = false;
              SetIsHeadingToNest(false);
              stRobotData.GoingToNest = false;
              stRobotData.GoingToOrFromNest = false;
              SetTarget(ReturnPatternPosition);
              DSA = SEARCHING;
    
          }
            
      }

  if(stRobotData.pathcheck == true)
  {
      
      stRobotData.pathcheck = false;
      stRobotData.Waypoint_Added = false;
//      SetMovement();
  }
    
//  else if(loopFunctions->FirstCheck == 1 and  loopFunctions->NewWayPointAdded == 1)
//  {
//      SetHardStopMovement();
//  }
//
//  else if(loopFunctions->FirstCheck == 1 && stRobotData.Waypoint_Added == true && stRobotData.pathcheck == false)
//  {
//      SetHardStopMovement();
//  }
    

  if(stRobotData.StopMovement == true)
  {
      SetStopMovement();
  }
    
  Move();
    
}

/*****
 * Sets target North of the robot's current target.
 *****/
void DSA_controller::SetTargetN(char x)
{
    CVector2 position = GetTarget();
    SetIsHeadingToNest(false);
    SetTarget(CVector2(position.GetX()+SearcherGap,position.GetY()));
}

/*****
 * Sets target South of the robot's current target.
 *****/
void DSA_controller::SetTargetS(char x){
    CVector2 position = GetTarget();
    SetIsHeadingToNest(false);
    SetTarget(CVector2(position.GetX()-SearcherGap,position.GetY()));
}

/*****
 * Sets target East of the robot's current target.
 *****/
void DSA_controller::SetTargetE(char x){
   CVector2 position = GetTarget();
   SetIsHeadingToNest(false);
   SetTarget(CVector2(position.GetX(),position.GetY()-SearcherGap));
}

/*****
 * Sets target West of the robot's current target.
 *****/
void DSA_controller::SetTargetW(char x){
    CVector2 position = GetTarget();
    SetIsHeadingToNest(false);
    SetTarget(CVector2(position.GetX(),position.GetY()+SearcherGap));
}

/*****
 * Helper function that reads vector <char> pattern
 * and sets the target's direction base on the 
 * char at the current vector index.
 *****/
 void DSA_controller::GetTargets(){

   /* If the robot hit target and the patter size >0
       then find the next direction. */
     
//     if(FirstTimeSearch == 2)
//     {
//         // set the flag to activate path planning algorithm
//         stRobotData.Checked = true;
//     }
    
    if(TargetHit() == true && tempPattern.size() > 0) {
        
    if(FirstTimeSearch < StepsToActivateAlgorithm)
    {
              FirstTimeSearch++;
    }

      /* Finds the last direction of the pattern. */
      direction_last = tempPattern[tempPattern.size() - 1];

        switch(direction_last)
        {
            case 'N':
                SetTargetN('N');
                break;
            case 'S':
                SetTargetS('S');
                break;
            case 'E':
                SetTargetE('E');
                break;
            case 'W':
                SetTargetW('W');
                break;
	}

	tempPattern.pop_back();
    stRobotData.pattern.pop_back();
        
    }
    
    else if(tempPattern.size() == 0) 
      {
    	Stop();
      }
}

/*****
 * Returns a boolean based on weather the robot is with 0.01 
 * distance tolerance. Declares that the robot had reached 
 * current target.
 *****/
 bool DSA_controller::TargetHit() {
    CVector2 position = GetPosition() - GetTarget();
    bool hit = false;
     
    if(position.SquareLength() < TargetDistanceTolerance){
        hit = true;
    }
    return hit;
 }

/*****
 * Check if the Robot is finding food. This is defined as the Robot being within
 * the distance tolerance of the position of a food item. If the Robot has found
 * food then the appropriate boolean flags are triggered.
 *****/
void DSA_controller::SetHoldingFood(){
    if(IsHoldingFood() == false) 
      {
	if(rand()*1.0/RAND_MAX < ProbTargetDetection) {
	
        vector <CVector2> newFoodList; 
        size_t i = 0;
        for (i = 0; i < loopFunctions->FoodList.size(); i++)
	  {
            if ((GetPosition()-loopFunctions->FoodList[i]).SquareLength() < FoodDistanceTolerance && !isHoldingFood)
	      {
              isHoldingFood = true;
              SetHardStopMovement();
	      } 
	    else 
	      {
              newFoodList.push_back(loopFunctions->FoodList[i]);
	      }
        } 
        loopFunctions->FoodList = newFoodList;
      }
      }

}

/*****
 * Is this Robot_controller holding food?
 *     true  = yes
 *     false = no
 *****/
bool DSA_controller::IsHoldingFood() {
    return isHoldingFood;
}
/*****
 * After pressing the reset button in the GUI, this controller will be set to
 * default factory settings like at the start of a simulation.
 *****/
void DSA_controller::Reset() {
    collisionDelay  = 0;
    SetIsHeadingToNest(true);
    SetTarget(loopFunctions->NestPosition);
    tempPattern.clear();
    CopyPatterntoTemp();
    generatePattern(NumberOfSpirals, NumberOfRobots);
    
}

REGISTER_CONTROLLER(DSA_controller, "DSA_controller")
