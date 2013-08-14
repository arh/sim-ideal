#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>    // std::sort
#include <vector>       // std::vector
using namespace std;

int main(int argc, char *argv[]) {
	
  if(argc != 3)
  {
    cout<<"USAGE: ./seq-length-analysis <inputFileName> <outputFileName>"<<endl;
    return 0;
  }
  
  string line;
  string subLine1;
  ifstream myfile (argv[1]);
  
  ofstream fout;
  fout.open(argv[2]); 
  //int threshold = 64;
  
  int lessLength=0;
  //int length=0;
  //int sumAboveThreshold=0;
  
  int total=0;
  
  int blockNumList[1000];
  
  int l1=0;
  int l2to3=0;
  int l4to7=0;
  int l8to15=0;
  int l16to31=0;
  int l32to63=0;
  int l64to127=0;
  int l128to255=0;
  int l256to511=0;
  int l512plus=0;  
  
  
  for(int j=0; j< 1000; j++)
		blockNumList[j]=0;
  
  //cout<<"Program running:)"<<endl;
  
  if (myfile.is_open())
  {
    while ( myfile.good() )
    {
      getline (myfile,line);
      if(line.length() < 10)
		continue;
	  
	  subLine1 = line.substr(45,50);
	  stringstream ss1(subLine1);
	  ss1>>lessLength;
	  //cout << lessLength << endl;
	  blockNumList[lessLength]++;
	}
    myfile.close();
  }
  else cout << "Unable to open file"; 
  
  
  for(int j=0; j< 1000; j++)
  {
	  total+=blockNumList[j];
	  if(blockNumList[j] == 0)
	  {
		  continue;
	  }
	  else
	  {	  
		  cout<<"length with "<<j<<" has number of "<<blockNumList[j]<<endl;
		  fout<<"length with "<<j<<" has number of "<<blockNumList[j]<<endl;
	  }
	  
	  if(j==1)
	  {
		  l1+=blockNumList[j];
	  }
	  if(j>=2&&j<=3)
	  {
		  l2to3+=blockNumList[j];
	  }
	  if(j>=4&&j<=7)
	  {
		  l4to7+=blockNumList[j];
	  }
	  if(j>=8&&j<=15)
	  {
		  l8to15+=blockNumList[j];
	  }
	  if(j>=16&&j<=31)
	  {
		  l16to31+=blockNumList[j];
	  }
	  if(j>=32&&j<=63)
	  {
		  l32to63+=blockNumList[j];
	  }
	  if(j>=64&&j<=127)
	  {
		  l64to127+=blockNumList[j];
	  }
	  if(j>=128&&j<=255)
	  {
		  l128to255+=blockNumList[j];
	  }
	  if(j>=256&&j<=511)
	  {
		  l256to511+=blockNumList[j];
	  }
	  if(j>=512)
	  {
		  l512plus+=blockNumList[j];
	  }
  }
  
  /*
  cout<<"length equal to 1 has the percentage of "<<double(l1)/double(total)*100<<"%"<<endl;
  cout<<"length between 2 and 3 has the percentage of "<<double(l2to3)/double(total)*100<<"%"<<endl;
  cout<<"length between 4 and 7 has the percentage of "<<double(l4to7)/double(total)*100<<"%"<<endl;
  cout<<"length between 8 and 15 has the percentage of "<<double(l8to15)/double(total)*100<<"%"<<endl;
  cout<<"length between 16 and 31 has the percentage of "<<double(l16to31)/double(total)*100<<"%"<<endl;
  cout<<"length between 32 and 63 has the percentage of "<<double(l32to63)/double(total)*100<<"%"<<endl;
  cout<<"length between 64 and 127 has the percentage of "<<double(l64to127)/double(total)*100<<"%"<<endl;
  cout<<"length between 128 and 255 has the percentage of "<<double(l128to255)/double(total)*100<<"%"<<endl;
  cout<<"length between 256 and 511 has the percentage of "<<double(l256to511)/double(total)*100<<"%"<<endl;
  cout<<"length equal and greater than 512 has the percentage of "<<double(l512plus)/double(total)*100<<"%"<<endl;
  * */
  
  fout<<"length equal to 1 has the percentage of "<<double(l1)/double(total)*100<<"%"<<endl;
  fout<<"length between 2 and 3 has the percentage of "<<double(l2to3)/double(total)*100<<"%"<<endl;
  fout<<"length between 4 and 7 has the percentage of "<<double(l4to7)/double(total)*100<<"%"<<endl;
  fout<<"length between 8 and 15 has the percentage of "<<double(l8to15)/double(total)*100<<"%"<<endl;
  fout<<"length between 16 and 31 has the percentage of "<<double(l16to31)/double(total)*100<<"%"<<endl;
  fout<<"length between 32 and 63 has the percentage of "<<double(l32to63)/double(total)*100<<"%"<<endl;
  fout<<"length between 64 and 127 has the percentage of "<<double(l64to127)/double(total)*100<<"%"<<endl;
  fout<<"length between 128 and 255 has the percentage of "<<double(l128to255)/double(total)*100<<"%"<<endl;
  fout<<"length between 256 and 511 has the percentage of "<<double(l256to511)/double(total)*100<<"%"<<endl;
  fout<<"length equal and greater than 512 has the percentage of "<<double(l512plus)/double(total)*100<<"%"<<endl;
  
  /*
  for(int j=threshold; j< 1000; j++)
  {
	  sumAboveThreshold+=blockNumList[j];
  }
  
  cout<<"sumAboveThreshold "<<sumAboveThreshold<<endl;
  fout<<"sumAboveThreshold "<<sumAboveThreshold<<endl;
  * */
  
  return 0;
}
