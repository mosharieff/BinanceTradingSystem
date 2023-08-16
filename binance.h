#ifndef BINANCE_H
#define BINANCE_H
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

/*
	Importing libraries
	
	The following libraries are required for the encryption and execution of the algorithmic orders. A shell script
	is included in order to successfully compile these libraries

*/
#include <iostream>
#include <sstream>
#include <map>
#include <cstring>
#include <cctype>
#include <openssl/hmac.h>
#include <curl/curl.h>
#include <chrono>
#include <cpprest/ws_client.h>
#include <thread>
#include <vector>
#include <algorithm>


#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "/usr/include/boost/variant.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/lexical_cast.hpp"
#include <boost/bind/bind.hpp>


// Namespaces are used to make calling the functions less tedious
using namespace boost::placeholders;

using namespace web;
using namespace web::websockets::client;


// This class holds all of the objects from the WebSocket feed to order placement REST requests 
class binance {

    private:
		
		// Key and Secret variables
        std::string api_key;
        std::string api_sec;

        // Websocket URL
        std::string ws_url = "wss://stream.binance.us:9443/ws";

        // API URL
        const std::string api_url = "https://api.binance.us";
        
        

        // Struct for writing response data into a string
        struct MemoryStruct {
            char* memory;
            size_t size;
        };
	
		// Timestamp which is called for all requests
        std::string nonce()
        {
            return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        }


        // Callback function to handle response data
        static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            size_t realsize = size * nmemb;
            MemoryStruct* mem = static_cast<MemoryStruct*>(userp);

            mem->memory = static_cast<char*>(realloc(mem->memory, mem->size + realsize + 1));
            if (mem->memory == nullptr) {
                std::cerr << "Not enough memory (realloc returned NULL)" << std::endl;
                return 0;
            }

            memcpy(&(mem->memory[mem->size]), contents, realsize);
            mem->size += realsize;
            mem->memory[mem->size] = 0;

            return realsize;
        }

        // Function to compute HMAC-SHA256 signature
        std::string sign(const std::map<std::string, std::string>& data, const std::string& secret) {
            std::ostringstream postdata;
            for (const auto& entry : data) {
                if (!postdata.str().empty()) {
                    postdata << "&";
                }
                postdata << entry.first << "=" << entry.second;
            }
            std::string message = postdata.str();
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int digest_len = 0;
            HMAC(EVP_sha256(), secret.c_str(), static_cast<int>(secret.length()),
                reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), digest, &digest_len);
            char mdString[2 * EVP_MAX_MD_SIZE + 1];
            for (unsigned int i = 0; i < digest_len; i++) {
                snprintf(&mdString[i * 2], 3, "%02x", static_cast<int>(digest[i]));
            }
            return mdString;
        }

        // Function to place post, get, and delete requests, requests with X as the verb are for unauthenticated pulling
        std::string request(std::string verb, std::string endpoint, std::map<std::string, std::string> data)
        {
        	// Declaration of request variable
            CURL * curl = curl_easy_init();
            
            struct MemoryStruct chunk;
            chunk.memory = static_cast<char*>(malloc(1));
            chunk.size = 0;
			
			// Declaration of headers
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + api_key).c_str());
			
			// Encrypts the order or account fetching
            std::string signature = sign(data, api_sec);
			
			// Builds a string with the parameters from the inputted map
            std::string postdata;
            for(auto & entry : data){
                if(!postdata.empty()){
                    postdata += "&";
                }
                postdata += entry.first + "=" + entry.second;
            }
            postdata += "&signature=" + signature;

            std::string url;
			
			// Various methods to set get and post and delete requests
            if(verb != "Y"){
                url = api_url + endpoint;
            }

            if(verb == "GET"){
                url += "?" + postdata;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            if(verb == "POST"){
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
            }
            if(verb == "DELETE"){
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&chunk));
			
			// Extracts the string 'response' from all of the characters after the request has been placed
            CURLcode res = curl_easy_perform(curl);
            std::string response(chunk.memory);

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            free(chunk.memory);
			
            return response;
        }
		
		// Parses all tickers and their respective base, quote, and WebSocket tickers using a map
        std::map<std::string, std::vector<std::string>> ParseInformation(boost::property_tree::ptree pt)
        {
            std::map<std::string, std::vector<std::string>> data;
			
			// Parses list
            auto get = [&](boost::property_tree::ptree pt)
            {
            	// Deposits all ticker data into a map
                auto look = [&](boost::property_tree::ptree pt)
                {
                    std::string temp;
                    boost::property_tree::ptree::const_iterator end = pt.end();
                    for(boost::property_tree::ptree::const_iterator it = pt.begin(); it != end; ++it){
                        if(it->first == "symbol"){
                            data["Tickers"].push_back(it->second.get_value<std::string>());
                            temp = "";
                            for(char cx : it->second.get_value<std::string>()){
                                temp += std::tolower(cx);
                            }
                            data["WSTickers"].push_back(temp);
                        }
                        if(it->first == "baseAsset"){
                            data["Base"].push_back(it->second.get_value<std::string>());
                        }
                        if(it->first == "quoteAsset"){
                            data["Quote"].push_back(it->second.get_value<std::string>());
                        }
                    }
                };
                boost::property_tree::ptree::const_iterator end = pt.end();
                for(boost::property_tree::ptree::const_iterator it = pt.begin(); it != end; ++it){
                    look(it->second);
                }
            };
            boost::property_tree::ptree::const_iterator end = pt.end();
            for(boost::property_tree::ptree::const_iterator it = pt.begin(); it != end; ++it){
                if(it->first == "symbols"){
                    // If the key is symbols, the value is passed to the get function which loops through it again
                    // until all variables have been extracted
                    get(it->second);
                }
            }

            return data;
        }
		
		// Parses the highest bid and lowest ask with their respective volumes
        void Cyclone(boost::property_tree::ptree pt, std::map<std::string, std::map<std::string, std::vector<std::string>>> & main_frame)
        {
            std::string base, quote, ticker;
            int look = 0;
            boost::property_tree::ptree::const_iterator end = pt.end();
            boost::property_tree::ptree::const_iterator begin = pt.begin();
            for(boost::property_tree::ptree::const_iterator it = begin; it != end; ++it){
                if(it->first == "s"){
                	// Matches the symbol in the WebSocket stream with the Information tickers and uses the index
                	// to find the base and quote pair
                    ticker = it->second.get_value<std::string>();
                    auto qt = std::find(tickers["Tickers"].begin(), tickers["Tickers"].end(), ticker);
                    look = std::distance(tickers["Tickers"].begin(), qt);
                    base = tickers["Base"][look];
                    quote = tickers["Quote"][look];
                    main_frame[base][quote] = {};
                }
                if(it->first == "b" || it->first == "B" || it->first == "a" || it->first == "A"){
                	// Adds the latest bids and asks to the main_frame vector where all of the data
                	// we will be using is stored
                    main_frame[base][quote].push_back(it->second.get_value<std::string>());
                }
            }
        }
		
		// Converts string output to JSON
        boost::property_tree::ptree json(std::string message)
        {
            boost::property_tree::ptree hold;
            std::stringstream ss;
            ss << message;
            boost::property_tree::read_json(ss, hold);
            
            return hold;
        }

    public:
		
		// Allows the key and secret to be inputted to this class		
        binance(std::string key, std::string sec){
            api_key = key;
            api_sec = sec;
        };
		
		// Trading variables to be passed through trading functions
        std::string BUY = "BUY", SELL = "SELL";

		// Fetches account data
        std::string Account()
        {
            std::string endpoint = "/api/v3/account";
            std::map<std::string, std::string> data = {
                {"timestamp", nonce()}
            };
            std::string resp = request("GET", endpoint, data);
            return resp;
        }
		
		// Places long or short limit orders
        std::string LimitOrder(std::string symbol, std::string side, double price, double volume)
        {
            std::string endpoint = "/api/v3/order";
            std::map<std::string, std::string> data = {
                {"symbol", symbol},
                {"side", side},
                {"type", "LIMIT"},
                {"timeInForce", "GTC"},
                {"quantity", std::to_string(volume)},
                {"price", std::to_string(price)},
                {"timestamp", nonce()}
            };
            std::string resp = request("POST", endpoint, data);
            return resp;
        }
		
		// Places market orders
        std::string MarketOrder(std::string symbol, std::string side, double volume)
        {
            std::string endpoint = "/api/v3/order";
            std::map<std::string, std::string> data = {
                {"symbol", symbol},
                {"side", side},
                {"type", "MARKET"},
                {"timeInForce", "GTC"},
                {"quantity", std::to_string(volume)},
                {"timestamp", nonce()}
            };
            std::string resp = request("POST", endpoint, data);
            return resp;
        }
		
		// Cancels an order with an inputted orderId
        std::string CancelOrder(std::string symbol, std::string orderId)
        {
            std::string endpoint = "/api/v3/order";
            std::map<std::string, std::string> data = {
                {"symbol", symbol},
                {"orderId", orderId},
                {"timestamp", nonce()}
            };
            std::string resp = request("DELETE", endpoint, data);
            return resp;
        }
		
		// Imports all ticker information for every tradeable pair on Binance.US
        std::string Information()
        {
            std::string endpoint = "/api/v3/exchangeInfo";
            std::map<std::string, std::string> data = {
                {"timestamp", nonce()}
            };
            std::string resp = request("X", endpoint, data);
            return resp;
        }
		
		// Parses the Information string into a JSON object
        std::map<std::string, std::vector<std::string>> INFORMATION()
        {
            std::string info = Information();
            return ParseInformation(json(info));
        }
		
		// tickers extracted into maps from the Information string
        std::map<std::string, std::vector<std::string>> tickers = INFORMATION();
        
		
		// WebSocket client
        static void SocketFeed(binance bx, std::map<std::string, std::map<std::string, std::vector<std::string>>> & main_frame)
        {
        	// Builds a message of at least 300-500 currency pairs
            std::string msgx = "";
            for(auto & t : bx.tickers["WSTickers"]){
                msgx += "/" + t + "@bookTicker";
            }
			
			// Connects to the WebSocket
            websocket_client client;
            client.connect(bx.ws_url + msgx).wait();
			
			// Streams socket data
            while(true){
                client.receive().then([](websocket_incoming_message msg)
                {
                    return msg.extract_string();
                }).then([&](std::string message)
                {	
                	// Message is parsed into the main_frame data map using the Cyclone function
                    bx.Cyclone(bx.json(message), std::ref(main_frame));
                }).wait();
                
            }
			
			// Closes the client if any errors arise
            client.close().wait();
        }
    
};



#endif
