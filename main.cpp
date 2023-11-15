#include<iostream>
#include<fstream>
#include<iomanip>
using namespace std;
#include<vector>
#include<map>

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
unsigned int Section_Start_Preprocess(vector<unsigned char> &buffer, size_t i);
void parse_DQT(vector<unsigned char> &buffer, size_t i);
void parse_DHT(vector<unsigned char> &buffer, size_t i);
void parse_SOS(vector<unsigned char> &buffer, size_t i);
void parse_SOF(vector<unsigned char> &buffer, size_t i);

/********************************************
 *    define some global data structure     *                                                                
 *******************************************/
int QuantTable[4][128] = {};  // QT最多四個, 每個64 or 128元素
map< pair< unsigned char, unsigned int >, unsigned char> HuffmanTable[2][2];





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
    unsigned int sectionSize = 0;
    for(size_t i = 0; i < buffer.size(); ++i){
        if( buffer[i] == 0xFF ){
            if( i + 1 >= buffer.size() ) break;
            //*************marker is the byte next FF*************
            unsigned char marker = buffer[++i];
            switch(marker){
                case SOI_MARKER: // SOI
                    cout << "********************Start of Image**********" << endl;
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
                    cout << "********************APP**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    i += (sectionSize); //將指標移到區段最後
                    break;
                case DQT_MARKER: // DQT
                    cout << "********************量化表**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    parse_DQT(buffer,i);
                    i += (sectionSize);
                    break;
                case DHT_MARKER: // DHT
                    cout << "********************霍夫曼編碼表**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    parse_DHT(buffer,i);
                    i += (sectionSize);
                    break; 
                case COM_MARKER: // COM 跳過
                    cout << "********************COMMENT**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    i += (sectionSize);
                    break; 
                case SOF_MARKER: // SOF
                    cout << "********************Start of frame**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    i += (sectionSize);
                    break;
                case SOS_MARKER: // SOS
                    cout << "********************Start of scan**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    i += (sectionSize);
                    break; 
                case EOI_MARKER: // EOI
                    cout << "********************End of Image**********" << endl;
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
unsigned int Section_Start_Preprocess(vector<unsigned char> &buffer, size_t i){
    unsigned int Section_Size =
    static_cast<unsigned int>(buffer[++i])*256 + 
    static_cast<unsigned int>(buffer[++i]);
    // i 現在指向第二個區段長度的byte
    cout << "Section size : " << Section_Size << endl;
    return Section_Size;
}
void parse_DQT(vector<unsigned char> &buffer, size_t i){
    /*******************讀取精度 && id***********************/
    i += 3; // 從DQT區段第一個byte開始(即2bytes長度之後第一個byte，記錄精度及id)
    unsigned char precision;
    precision = (buffer[i] >> 4 == 0) ? 8 : 16; // 第一個byte前四bits是精度
    unsigned char id; 
    id = buffer[i] & 0x0F; // 後四bits是量化表id，與 0000 1111 做bitwise and之後可得後四bits
    cout << "量化表ID: " << static_cast<int>(id) << " , 量化表精度: " << static_cast<int>(precision) << endl;
    ++i;   // 指向資料開始的地方

    /*******************讀取QT data  ***********************/
    precision /= 8;                                // 精度為 1 or 2
    int table_size = (precision == 1) ? 64 : 128;  // 大小為 64 or 128
    cout << "table size: " << table_size <<endl;
    // j 從0到table size - 1, 
    // k 從 i 到 i + table size -1 
    for(int j = 0, k = i; k < i + table_size; ++j, ++k)
        QuantTable[id][j] = buffer[k];

    for(int k = 0; k < table_size; ++k){
        if ( k % 8 == 0 ) cout << endl;
        cout << setw(2) << setfill(' ') << QuantTable[id][k] << " ";
    }
    cout << endl;
}

void parse_DHT(vector<unsigned char> &buffer, size_t i){
    i += 3; // 指向2bytes長度後面那個byte(DC/AC,id)

    /*****************判斷DC/AC及id****************/
    unsigned char type = (buffer[i] >> 4 == 0) ? 0 : 1;
    unsigned char id = buffer[i] & 0x0F;
    if(type) cout << "AC-" << static_cast<int>(id) << endl;
    else cout << "DC-" << static_cast<int>(id) << endl;
 
    ++i; // 現在i指向16bytes的第一個bytes

    /******************計算葉節點個數****************/
    unsigned int leaf_number = 0;
    unsigned char leaf_distribution[16];
    for(int j = 0; j < 16; ++j, ++i){
        leaf_distribution[j] = buffer[i];
        leaf_number += buffer[i];
    }
    // 現在i指向解碼資料的第一個byte

    cout << "leaf distribution : ";
    for (int n : leaf_distribution)
        cout << n << " ";
        
    cout << endl << "leaf numbers : " << leaf_number << endl;

    
    /******************處理霍夫曼表****************/
    // 先做出長度&編碼pair
    auto len_code = new pair<unsigned char, unsigned int>[leaf_number];
    int code = 0;
    int current = 0;
    for (int j = 0; j < 16; ++j){
        for (int k = 0; k < leaf_distribution[j]; ++k){
            len_code[current++] = make_pair( j + 1, code);
            ++code;
        }
        code = code << 1;
    }
    // for ( int j = 0; j < leaf_number; ++j)
    //     cout << int(len_code[j].first) << " : " << len_code[j].second << endl;
    
    //把長度&編碼pair and the corresponding plain text put in HuffmanTable
    for ( int j = 0; j < leaf_number; ++j, ++i){
        HuffmanTable[type][id][len_code[j]] = buffer[i];
        cout << (int)len_code[j].first << "  " << len_code[j].second << " : " << (int)(HuffmanTable[type][id][len_code[j]]) << endl;
    }
    delete[] len_code;
}
void parse_SOS(vector<unsigned char> &buffer, size_t i){}
void parse_SOF(vector<unsigned char> &buffer, size_t i){}













