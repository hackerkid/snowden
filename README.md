# snowden (HTTP Proxy Serever) Docs
Yet another prooxy server in C++

##Installing

* Extract the compressed file
* run `make` in terminal for compiling and building the binary
* run `./proxy Port` to start the proxy server in the desired PORT

###IIITA Proxy
* If you are testing the proxy server in IIIT-A network then change the value of `CONNECT_TO_IRONPORT` defined in line 15 proxy.cpp to `true`. 
* If you are using mobile data then there is no need of chaning the value of `CONNECT_TO_IRONPORT`. Leave it as `false`. 
* The python tests would only work under mobile data.  

##Design 

The proxy server would act as both a client and server. 
![image](https://www.drupal.org/files/project-images/proxy.png)

* Each client would connect to the proxy server which in turn fetches the desired web page back to the client.
* To support multiple clients I used thread programming (pthread library). 
* The request made to the proxy server by the client can be validated with the help of `ParsedRequest_parse` function in `proxy_parse.h`. 
* HTTPS is not supported becuase the data send between the client and server is heavily encrypted. No sense can be made of data and it would be useless to cache anything in the proxy server.


##Testing
Go to the `Edit` menu in Firefox.
* Select `Preferences`. Select `Advanced` and then select `Network.
* Under `Connection`, select `Settings`.
* Select `Manual Proxy Configuration`. If you are using localhost, remove the
default `No Proxy for: localhost 127.0.0.1`. Enter the hostname (eg localhost) and port where
your proxy program is running.
* Save your changes by selecting `OK` in the connection tab and then select `Close`
in the preferences tab.




