#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <vector>
#include <cstring>
#include <cctype>
#include <algorithm>
#include "binance.h"
#include <ctime>
#include <fstream>

// Designed to sleep the main thread while the WebSocket thread loads data in tbe background
void SLEEP(int seconds)
{
	std::time_t t = std::time(0);
	while(true){
        std::cout << seconds - (std::time(0) - t) << " left" << std::endl;
		if(std::time(0) - t > seconds){
			break;
		}
	}    

}

// Primary data map used to store all currency information
std::map<std::string, std::map<std::string, std::vector<std::string>>> main_frame;
   
// This is the function which holds your key and secret strings
std::map<std::string, std::string> Auth(){
    std::string key = "";
    std::string secret = "";
    std::map<std::string, std::string> res;
    res["key"] = key;
    res["secret"] = secret;
    return res;
}

// Finds the index of an element in a vector, returns -1 if element cannot be found
int findex(const std::vector<std::string>& myVector, std::string targetElement) {
    for (size_t i = 0; i < myVector.size(); ++i) {
        if (myVector[i] == targetElement) {
            return static_cast<int>(i);
        }
    }
    return -1;  // Element not found
}


// Main root of the whole program
int main() {
    
    std::map<std::string, std::string> auth = Auth(); // Fetches Key and Secret

    std::ofstream writer("log.csv"); // Opens a writer to write the log to a .csv file
    
    binance bx(auth["key"], auth["secret"]); // Passes key and secret to the binance class denoted bx

    std::thread oc(bx.SocketFeed, bx, std::ref(main_frame)); // Initializes the WebSocket feed in a thread. bx is passed due to the socket being a static void function
	
	// Declare all of the neccesary variables
    std::string base, quote, pair;
	
    int ii = 0, jj = 0, kk = 0;

    double price = 0, synthetic = 0, diff = 0, balance = 1.0;
    double sdiff = 0;
	
	// Sleeps while threaded socket loads data
    SLEEP(120);
	
	// Transaction Fee
    double tx = 0.45/100.0;
    double txI = (1 + tx), txJ = (1 - tx);
    double v0, v1, v2;
	
	// Writes headers of .csv log file
    writer << "Time,Side,Pair,Price,Side,Pair,Price,Side,Pair,Price,Balance\n";
    writer.flush();
	
	// Runs the system
    while(true){
    
    	// Loops through each crypto currency in order to find arbitrage. For example entry.first = ADA, this looks for 
    	// any arbitrage in Cardano
    	
        for(auto & entry : main_frame){
            if(true){
                ii = 0;
                
                // Fetches base and quote
                
                for(auto & u : main_frame[entry.first]){
                    jj = 0;
                    
                    // Fetches base and quote
                    
                    for(auto & v : main_frame[entry.first]){
                    
                    	// Checks if the base and quote are not equal, and the pairs are in the upper triangular portion 
                    	// of the matrix
                    	
                        if(u.first != v.first && ii < jj){
                        
                        	// Checks to see if the pair being compared to the synthetic exists
                            pair = u.first + v.first;
                            kk = findex(bx.tickers["Tickers"], pair);
                            if(kk >= 0){
                            
                            	// If pair exists, extract the base and quote
                                base = bx.tickers["Base"][kk];
                                quote = bx.tickers["Quote"][kk];
                                
                                // Checks to see that all of the vectors are not currently being modified in the thread in order to minimize errors and segfaults
                                if(main_frame[base][quote].size() > 0 && main_frame[entry.first][v.first].size() > 0 && main_frame[entry.first][u.first].size() > 0){
                                
                                    // Calculates the synthetic pair
                                    synthetic = std::stod(main_frame[entry.first][v.first][0])/std::stod(main_frame[entry.first][u.first][2]);
                                    
                                    // Fetches the price of the pair being compared to the synthetic pair
                                    price = std::stod(main_frame[base][quote][0]);
                                    
                                    if(true){
                                    	// Trade simulation starts off with 10 units, if the number of units increase after the trade, it is logged as profitable
                                        balance = 12;
                                        balance = (balance/std::stod(main_frame[entry.first][v.first][2]))*txJ; // Buy SOLUSDC
                                        balance = (balance*std::stod(main_frame[entry.first][u.first][0]))*txJ; // Sell SOLBTC
                                        balance = (balance*std::stod(main_frame[base][quote][0]))*txJ;          // Sell BTCUSDC
                                        std::cout << entry.first << "\t" << price - synthetic << "\t" << v.first << "\t" << u.first << "\t" << balance << std::endl;
                                        
                                        // Logging of the positive trade to the log csv file
                                        if(balance >= 10){

                                            // Algorithmic Trading Order Placement
                                            balance = 12;

                                            // Calculate the initial volume and execute a buy order
                                            v0 = balance/std::stod(main_frame[entry.first][v.first][2]);
                                            bx.MarketOrder(entry.first+v.first, bk.BUY, v0);
                                            
                                            // Calculate the current volume and execute a sell order
                                            v1 = v0*std::stod(main_frame[entry.first][u.first][0]);
                                            bx.MarketOrder(entry.first+u.first, bk.SELL, v1);
                                            
                                            // Calculate the final volume and execute a sell order
                                            v2 = v1*std::stod(main_frame[base][quote][0]);
                                            bx.MarketOrder(base+quote, bk.SELL, v2);
                                            
                                            // Writes log to .csv
                                            writer << std::time(0) << "," << "Buy" << "," << entry.first+v.first << "," << main_frame[entry.first][v.first][2] << "," << "Sell" << "," << entry.first+u.first << "," << main_frame[entry.first][u.first][0] << "," << "Sell" << "," << base+quote << "," << main_frame[base][quote][0] << "," << balance << "\n";
                                            writer.flush();
                                        }
                                    }
                                    
                                }
                                
                            }
                        }
                        
                        jj += 1;
                    }
                    ii + 1;
                }
            }
        }

    }
	
	// Joins to end the socket thread
    oc.join();

    return 0;
}



