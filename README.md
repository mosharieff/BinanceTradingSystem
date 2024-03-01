# BinanceTradingSystem :game_die:

## Important Note :memo:
The strategy in this system is not profitable with the existing transaction costs of Binance.US. However, the system streams data and provides trading signals via WebSocket. Additionally the account balance fetcher, the order placement function, and the order cancellation functions all work. DO NOT USE THIS SYSTEM WITH REAL MONEY.

## Description :red_circle:
I built this system in order to capture arbitrage between different cryptocurrencies trading on Binance.US. When an arbitragable pair has been found, the program writes the output into a .csv file. For example a trade might look something like this:
<br/>

ETHUSD = $1000, ETHBTC = 0.05, BTCUSD = $25000
<br/>
<br/>
If we divide ETHUSD/ETHBTC we get a synthetic BTCUSD pair. In this example that synthetic pair is 1000/0.05 = $20000, which is less than the actual BTCUSD price of $25000. For example lets start off our balance with $12.00:
<br/>
1. Buy ETHUSD with $12.00, we hold 12/1000 = 0.012 ETH
2. Sell ETHBTC with 0.012 ETH, we hold 0.012*0.05 = 0.0006 BTC
3. Sell BTCUSD with 0.0006 BTC, we hold 0.0006*25000 = $15.00
<br/>
This arbitrage trade netted us three dollars in profit (without transaction costs factored in)

## Installation and Imports :anchor:
The following libraries need to be installed according to whichever OS you are using
<br/>
1. CURL
2. OpenSSL
3. CryptoPP
4. CPPRest
5. Boost
<br/>
Once you have the libraries installed you may compile it as follows:
<br/>
> g++ main.cpp -std=c++11 -lcurl -lssl -lcrypto -lcpprest -lpthread
<br/>
> ./a.out
<br/>
This system is designed to run on Ubuntu Linux and is fully built with Linux. Further action may be required in order to run this on a Windows or Mac PC.

## Live Run :volcano:
![alt](https://github.com/mosharieff/BinanceTradingSystem/blob/main/RUN.GIF)
