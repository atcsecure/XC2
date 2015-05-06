#include "servermixer.h"
#include <cmath>

CCriticalSection cs_mapServerMixTxs;
std::map<int64, CServerMixTx> mapServerMixTxs;

bool withinRange(double first, double second, double range) {
  //std::cerr << "Difference is " << first - second << std::endl;
  //std::cerr << "Absolute value is " << fabs(first-second) << std::endl;
  //std::cerr << "Within range? " << (fabs(first-second) < range) << std::endl;
  if (fabs(first - second) < range) return true;
  return false;
}

void ServerMix(std::string iaddress, std::string target, double total, bool multipath) {

  int iSecret;
  int iLow;
  int iHigh;
  //std::cerr << "INSIDE THE MIXING LOGIC" << std::endl;
/* initialize random seed: */
  srand (time(NULL));
  iLow = 1;
  iHigh = 4;
  /* generate secret number between 1 and 10: */
  iSecret = 1 + (rand() % 5);



  // ensure smtx.amount coins was transferred to smtx.iaddress
  Array iargs;
  Array args;
  iargs.push_back(iaddress);
  iargs.push_back(iSecret); // randomized

  Value iamount;
  double min = ((double)MIN_TXOUT_AMOUNT)/((double)COIN);
  double sent = 0;
  double balance;
  double lastbalance = 0;

  //std::cerr << "Is " << total << " within " << min 
   //         << " range of " << sent << "?" << std::endl;

  while (!withinRange(total, sent, min)) {
    //std::cerr << "Trying to check mixer server received." << std::endl;
    try {
      iamount = getreceivedbyaddress(iargs, false);
    }
    catch (Object& e) {
      //std::cerr << "Failed!!!!";
      for (Object::iterator i = e.begin(); i != e.end(); ++i) {
        if ((*i).value_.type() == str_type)
          std::cerr << "!!! " << (*i).value_.get_str()<< std::endl;
      }
      break;
    }

    balance = iamount.get_real();
    double tosend = balance - lastbalance;
    //std::cerr << "Comparing balance of " << balance << " to old balance "
     //         << lastbalance << " with result of " << tosend <<  "." << std::endl; 

    if (tosend - min > 0) {
      //std::cerr << "Sending " << tosend - min << " from received amount " 
       //         << tosend << " to " << target << "." << std::endl
//		<< "Recieved total of " << balance << " out of expected "
//		<< total << std::endl;
      args.clear();
 //     args.push_back("mixer");   //requires Wallet called mixer
 //     args.push_back(target);
 //     args.push_back(tosend - min);
     // args.push_back(25);
      
//      if (mstx.usesendfrommixer)
//      {
//        try {
//               sendfrommixer(args, false);// test
//              }
//      else
//       try {
//	     sendfrom(args, false);
//            };
//}


    if (multipath) {

   try {
        //args.push_back(false);
      args.push_back("mixer");   //requires Wallet called mixer
      args.push_back(target);
      args.push_back(tosend - min);
      args.push_back(0); 
 	 sendfrommixer(args, false);
          }

      catch (Object& e) {
        //std::cerr << "Failed!!!!";
        for (Object::iterator i = e.begin(); i != e.end(); ++i) {
          if ((*i).value_.type() == str_type)
            std::cerr << "!!! " << (*i).value_.get_str()<< std::endl;
        }

       }}
     else { 
      try {
      args.push_back("mixer");   //requires Wallet called mixer
      args.push_back(target);
      args.push_back(tosend - min);

        sendfrom(args, false);
          }

      catch (Object& e) {
        //std::cerr << "Failed!!!!";
        for (Object::iterator i = e.begin(); i != e.end(); ++i) {
          if ((*i).value_.type() == str_type)
            std::cerr << "!!! " << (*i).value_.get_str()<< std::endl;
	}

       }

	break;
      }

      sent += tosend;
    }

    //std::cerr << "Updating old balance from " << lastbalance << " to " << balance << std::endl;
    lastbalance = balance;
    
    //std::cerr << "And sleep" << std::endl;
    boost::thread::sleep(boost::get_system_time() +
                         boost::posix_time::seconds(5));
  }

  //std::cerr << "Completed within range." << std::endl;
}
