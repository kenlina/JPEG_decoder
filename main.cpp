#include<iostream>
#include<fstream>
using namespace std;
#include<vector>

int main(int argc, char *argv[]){
    ifstream img("/home/ken/Desktop/112_1/ITCT/Image/gig-sn01.jpg",ios::binary);
    img.seekg(0,ios::end);
    size_t imgSize = img.tellg();
    img.seekg(0,ios::beg);

    if(!img){
        cerr << "Can't open image." << endl;
        return 1;
    }

    vector<unsigned char> buffer(imgSize);
    img.read(
        reinterpret_cast<char*> ( buffer.data() ),
        buffer.size()
    );
    for(unsigned char byte : buffer){
        cout << hex <<  static_cast<int>(byte) << " ";
    }
    cout << endl;

    cout << dec << "The image size is " << imgSize << endl;

    img.close();
    return 0;
}