#include<iostream>
#include<fstream>
using namespace std;
#include<vector>

void read_JPEG(char *argv, vector<unsigned char> &buffer);

int main(int argc, char *argv[]){
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <image path>" << endl;
        return 1;
    }
    //*************將讀到的資料存在buffer中***********
    vector<unsigned char> buffer;
    read_JPEG(argv[1],buffer);

    return 0;
}

void read_JPEG(char *argv, vector<unsigned char> &buffer){
    //**********Open image************
    ifstream img(argv,ios::binary); 

    //**********Record image length************
    img.seekg(0,ios::end);
    size_t imgSize = img.tellg();
    img.seekg(0,ios::beg);


    if(!img){
        cerr << "Can't open image." << endl;
        exit (1);
    }

    buffer.resize(imgSize);
    img.read(
        reinterpret_cast<char*> ( buffer.data() ),
        buffer.size()
    );
    // for(unsigned char byte : buffer){
    //     cout << hex <<  static_cast<int>(byte) << " ";
    // }
    // cout << endl;

    cout << dec << "The image size is " << imgSize << endl;

    img.close();
}