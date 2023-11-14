#include<iostream>
#include<fstream>
using namespace std;
#include<vector>

/********************************************
 *           define some constant           *                                                                
 *******************************************/
const int SOI_MARKER = 0xD8;
const int APP_MARKER = 0xE0;
const int DQT_MARKER = 0xDB;
const int SOF_MARKER = 0xC0;
const int DHT_MARKER = 0xC4;
const int SOS_MARKER = 0xDA;
const int EOI_MARKER = 0xD9;
const int COM_MARKER = 0xFE;



/********************************************
 *           define some functions          *                                                                
 *******************************************/
void read_JPEG(char *argv, vector<unsigned char> &buffer);
void parse_JPEG(vector<unsigned char> &buffer);
void Section_Start_Preprocess(vector<unsigned char> &buffer, size_t &i);





int main(int argc, char *argv[]){
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <image path>" << endl;
        return 1;
    }
    //*************將讀到的資料存在buffer中***********
    vector<unsigned char> buffer;
    read_JPEG(argv[1],buffer);
    parse_JPEG(buffer);
    return 0;
}

void parse_JPEG(vector<unsigned char> &buffer){
    for(size_t i = 0; i < buffer.size(); ++i){
        if( buffer[i] == 0xFF ){
            if( i + 1 >= buffer.size() ) break;
            //*************marker is the byte next FF*************
            unsigned char marker = buffer[++i];
            switch(marker){
                case SOI_MARKER: // SOI
                    cout << "Start of Image" << endl;
                    break;
                case APP_MARKER: // APP 跳過
                case 0xE1:
                case 0xE2:
                case 0xE3:
                case 0xE4:
                case 0xE5:
                case 0xE6:
                case 0xE7:
                case 0xE8:
                case 0xE9:
                case 0xEA:
                case 0xEB:
                case 0xEC:
                case 0xED:
                case 0xEE:
                case 0xEF:
                    cout << "APP" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break;
                case DQT_MARKER: // DQT
                    cout << "量化表" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break;
                case DHT_MARKER: // DHT
                    cout << "霍夫曼編碼表" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break; 
                case COM_MARKER: // COM 跳過
                    cout << "COMMENT" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break; 
                case SOF_MARKER: // SOF
                    cout << "Start of frame" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break;
                case SOS_MARKER: // SOS
                    cout << "Start of scan" << endl;
                    Section_Start_Preprocess(buffer,i);
                    break; 
                case EOI_MARKER: // EOI
                    cout << "End of Image" << endl;
                    break; // 結束解析
            }
        }
    }
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

    cout << dec << "The image size is " << imgSize << " bits." << endl;

    img.close();
}
void Section_Start_Preprocess(vector<unsigned char> &buffer, size_t &i){
    unsigned int Section_Size =
    static_cast<unsigned int>(buffer[++i])*256 + 
    static_cast<unsigned int>(buffer[++i]);
    // i 現在指向第二個區段長度的byte
    cout << "Section size : " << Section_Size << endl;
    i += (Section_Size - 2);
}