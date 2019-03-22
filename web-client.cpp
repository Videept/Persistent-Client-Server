#include "HTTPRequest.hpp"
#include <bits/stdc++.h>
#include <fstream>
#include<iostream>

int main(int argc,char** argv){
try
{
    http::Request firstreq(argv[1]);	//Taking the first URL as a request

    // send a get request
    http::Response resp = firstreq.send("GET");	//Taking the entire GET request
    std::cout << resp.body.data() << std::endl; // print the result
   

   std::ofstream output1("result.html");
  output1<<resp.body.data();
  output1.close();

if(argc>2)
{
	http::Request secondreq(argv[2]);
 	http::Response resp1 = secondreq.send("GET");
    	std::cout << resp1.body.data() << std::endl; // print the result
 
	std::ofstream output2("result2.html");
	//std::cout<<secondreq.path();
  	output2<<resp1.body.data();
  	output2.close();
}

if(argc>3)
{
	http::Request thirdreq(argv[3]);
 	http::Response resp2 = thirdreq.send("GET");
    	std::cout << resp2.body.data() << std::endl; // print the result
 
	std::ofstream output3("result3.html");
	//std::cout<<thirdreq.path();
  	output3<<resp2.body.data();
  	output3.close();
}
 

    // send a get request
  //  http::Response response1 = request1.send("GET");
    //std::cout << response1.body.data() << std::endl; // print the result

    // send a post request
   // response = request.send("POST", "foo=1&bar=baz", {
     //   "Content-Type: application/x-www-form-urlencoded"
   // });
   // std::cout << response.body.data() << std::endl; // print the result

    // pass parameters as a map
   // std::map<std::string, std::string> parameters = {{"foo", "1"}, {"bar", "baz"}};
    //response = request.send("POST", parameters, {
      //  "Content-Type: application/x-www-form-urlencoded"
   // });
   // std::cout << response.body.data() << std::endl; // print the result
//while(1);
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << std::endl;
}
return 0;
}
