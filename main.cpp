#include<iostream>
#include<fstream>
#include<iomanip>
using namespace std;
#include<vector>
#include<map>
#include<cmath>

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
 *    define some global data structure     *                                                                
 *******************************************/
int QuantTable[4][128] = {};  // QT最多四個, 每個64 or 128元素
map< pair< unsigned char, unsigned int >, unsigned char> HuffmanTable[2][2];// DC or AC, id 0/1

/*顏色分量信息*/
struct JPEGcomponent{
    uint8_t ColorID;
    uint8_t horizontalSampling;
    uint8_t verticalSampling;
    uint8_t QuantTableID;
};
/*SOF0*/
struct SOF{
    uint8_t  precision;
    uint16_t height;
    uint16_t width;
    vector<JPEGcomponent> component;
    SOF():component(4){}// 扣掉id0不用id 1,2,3代表Y,Cb,Cr
} SOF0;
uint16_t maxHorizontalSampling = 0;
uint16_t maxVerticalSampling = 0;
uint16_t SOStable[4][2]; // 扣掉id0不用id 1,2,3代表Y,Cb,Cr, 每個顏色裡面有兩個元素0是DC的霍夫曼id,1是AC的霍夫曼id

// 定義block為8*8的double
typedef double BLOCK[8][8];

struct ACcoefficient {
    uint16_t zeros;
    uint16_t length;
    int value;
};
class MCU{
public:
    BLOCK mcu[4][2][2];// id對應顏色 id0不用

};

double cos_cache[200];
const double PI = 3.14159265358979323846;
struct RGB
{
    int R,G,B;
};






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
void parse_DATA(vector<unsigned char> &data);
int readBit(vector<unsigned char> &data);
int getDClenth(vector<unsigned char> &data, int ColorID);
int readDCvalue(vector<unsigned char> &data,int ColorID);
unsigned char getACinfo(vector<unsigned char> &data, int ColorID);
ACcoefficient readACvalue(vector<unsigned char> &data,int ColorID);
MCU readMCU(vector<unsigned char> &data);
void showMCU(MCU curMCU);
void inverseQuantify(MCU &curMCU);
MCU inverseZigzag(MCU &curMCU);
void iDCT(MCU &curMCU);
vector<vector<RGB>> YCbCrtoRGB(MCU &curMCU);





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
    vector<unsigned char> data;
    for(size_t i = 0; i < buffer.size(); ++i){
        if( buffer[i] == 0xFF ){
            if( i + 1 >= buffer.size() ) break;
            //*************marker is the byte next FF*************
            unsigned char marker = buffer[++i];
            // i 現在指向marker
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
                    parse_DQT(buffer,i);
                    i += (sectionSize);
                    break;
                case DHT_MARKER: // DHT
                    cout << "********************霍夫曼編碼表**********" << endl;
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
                    parse_SOF(buffer,i);
                    i += (sectionSize);
                    break;
                case SOS_MARKER: // SOS
                    cout << "********************Start of scan**********" << endl;
                    sectionSize = Section_Start_Preprocess(buffer,i);
                    parse_SOS(buffer,i);
                    i += (sectionSize + 1); // i現在指向壓縮數據的第一個byte
                    data.insert( data.begin(), buffer.begin() + i, buffer.end() );
                    parse_DATA(data);
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
    size_t section_start = i + 1; // 從長度的第一個byte開始計 
    unsigned int section_length = Section_Start_Preprocess(buffer, i);
    i += 3; // 從DQT區段第一個byte開始(即2bytes長度之後第一個byte，記錄精度及id)

    while (i < section_start + section_length) {
        /*******************讀取精度 && id***********************/
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
        i += table_size; // 將 i 指向下一個(如果有的話)table的開始
    }
}
void parse_DHT(vector<unsigned char> &buffer, size_t i){
    size_t section_start = i + 1; // 長度從兩byte長度的第一個byte開始計
    unsigned int section_length = Section_Start_Preprocess(buffer, i);
    i += 3; // 指向2bytes長度後面那個byte(DC/AC,id)

    while (i < section_start + section_length) {
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
        //把長度&編碼pair and the corresponding plain text put in HuffmanTable
        for ( int j = 0; j < leaf_number; ++j, ++i){
            HuffmanTable[type][id][len_code[j]] = buffer[i];
            // cout << (int)len_code[j].first << "  " << len_code[j].second << " : " 
            // << (int)(HuffmanTable[type][id][len_code[j]]) << endl;
        }
        delete[] len_code;
    }
}
void parse_SOS(vector<unsigned char> &buffer, size_t i){
    i += 4; // i現在指向第一個顏色id
    for (int tmp = i; tmp < i + 6; tmp += 2){
        SOStable[buffer[tmp]][0] = buffer[tmp + 1] >> 4;
        SOStable[buffer[tmp]][1] = buffer[tmp + 1] & 0x0F; 
    }
    for(int j = 1; j < 4; ++j){
        cout<< "顏色ID: " << j << ", DC對應霍夫曼ID: " << SOStable[j][0] << ", AC對應霍夫曼ID: " << SOStable[j][1] << endl;
    }

}
void parse_SOF(vector<unsigned char> &buffer, size_t i){
    i += 3; // i現在指向兩byte長度後的下一個byte 即精度
    /*存入精度*/
    SOF0.precision = buffer[i];

    /*compute height*/
    uint16_t height = buffer[++i] * 256 + buffer[++i];
    //  i現在指向兩byte高度的第二個byte
    SOF0.height = height;

    /*compute width*/
    uint16_t width = buffer[++i] * 256 + buffer[++i];
    //  i現在指向兩byte寬度的第二個byte
    SOF0.width = width;

    i += 2; //  i現在指向9 bytes 的第1個byte
    /*存入component*/
    for(int tmp = i; tmp < i + 9; tmp += 3){
        SOF0.component[buffer[tmp]].ColorID = buffer[tmp];
        SOF0.component[buffer[tmp]].horizontalSampling = buffer[tmp + 1] >> 4;
        SOF0.component[buffer[tmp]].verticalSampling = buffer[tmp + 1] & 0x0F; 
        SOF0.component[buffer[tmp]].QuantTableID = buffer[tmp+2];
        if(SOF0.component[buffer[tmp]].horizontalSampling > maxHorizontalSampling)
        maxHorizontalSampling = SOF0.component[buffer[tmp]].horizontalSampling;
        if(SOF0.component[buffer[tmp]].verticalSampling > maxVerticalSampling)
        maxVerticalSampling = SOF0.component[buffer[tmp]].verticalSampling;
    }
    cout << "精度: " << (int)SOF0.precision << endl;
    cout << "圖高: " << SOF0.height << endl;
    cout << "圖寬: " << SOF0.width << endl; 
    cout << "Y ->  ID:" << (int)SOF0.component[1].ColorID << " 水平採樣率:" << (int)SOF0.component[1].horizontalSampling 
    << " 垂直採樣率:" << (int)SOF0.component[1].verticalSampling << " 量化表ID:" << (int)SOF0.component[1].QuantTableID << endl;
    cout << "Cb->  ID:" << (int)SOF0.component[2].ColorID << " 水平採樣率:" << (int)SOF0.component[2].horizontalSampling 
    << " 垂直採樣率:" << (int)SOF0.component[2].verticalSampling << " 量化表ID:" << (int)SOF0.component[2].QuantTableID << endl;
    cout << "Cr->  ID:" << (int)SOF0.component[3].ColorID << " 水平採樣率:" << (int)SOF0.component[3].horizontalSampling 
    << " 垂直採樣率:" << (int)SOF0.component[3].verticalSampling << " 量化表ID:" << (int)SOF0.component[3].QuantTableID << endl;

    cout << endl << "最大水平採樣率： " << maxHorizontalSampling << endl;
    cout << "最大垂直採樣率： " << maxVerticalSampling << endl;
}
/*
    4:4:4： 
    如果所有分量的 H 和 V 採樣因子都相同，例如都是 1/1，則採樣格式為 4:4:4。
    這表示 Y、Cb 和 Cr 分量都是以同樣的率採樣。在這種情況下，每個 MCU 包含 1 塊 Y、1 塊 Cb 和 1 塊 Cr，共 3 塊。
    MCU 的大小是 8x8 像素。

    4:2:2： 
    如果 Y 分量的水平採樣率是 Cb 和 Cr 的兩倍，垂直採樣率相同，例如 Y 為 2/1，Cb 和 Cr 為 1/1，則採樣格式為 4:2:2。
    這表示 Y 分量是色度分量的兩倍採樣率（只在水平方向）。每個 MCU 包含 2 塊 Y、1 塊 Cb 和 1 塊 Cr，共 4 塊。
    MCU 的大小是 16x8 像素（水平方向兩倍）。

    4:2:0： 
    如果 Y 分量的水平和垂直採樣率都是 Cb 和 Cr 的兩倍，例如 Y 為 2/2，Cb 和 Cr 為 1/1，則採樣格式為 4:2:0。
    這是最常見的採樣格式，其中 Y 分量的採樣率是色度分量的兩倍（在水平和垂直方向）。
    在這種情況下，每個 MCU 包含 4 塊 Y、1 塊 Cb 和 1 塊 Cr，共 6 塊。
    MCU 的大小是 16x16 像素（水平和垂直方向都兩倍）。
    
*/
void parse_DATA(vector<unsigned char> &data){
    /*************************先計算需要多少MCU再來解碼MCU***************************/
    double vMCU = ceil(double(SOF0.height)/double(maxVerticalSampling*8));
    double hMCU = ceil(double(SOF0.width)/double(maxHorizontalSampling*8));
    cout << "圖片高度：" << SOF0.height << " 圖片寬度：" << SOF0.width <<endl;
    cout << "MCU高度 ： " << maxVerticalSampling*8 << "  MCU寬度： " << maxHorizontalSampling*8 <<endl;
    cout << "垂直MCU ： " << vMCU << "  水平MCU： " << hMCU <<endl;

    /****************************開讀****************************/
    cout << "********************開始讀取MCU********************"<<endl;
    for( int v = 0; v < vMCU; ++v)
        for( int h = 0; h < hMCU; ++h){
            MCU curMCU = readMCU(data);
            inverseQuantify(curMCU);
            curMCU = inverseZigzag(curMCU);
            iDCT(curMCU);
            showMCU(curMCU);
        }
}
int readBit(vector<unsigned char> &data ){
    static int bytePos = 0;
    static int bitPos = 0;
    if (bytePos == data.size() - 2 ) {
        cerr << "Reached end of image by carry out the bytePos." << endl;
        return -1;  
    }
    if(bytePos > 0 && data[bytePos] == 0x00 && data[bytePos - 1] == 0xFF) 
        ++bytePos; // 出現0xFF00的狀況就跳過0x00
    if (bytePos >= data.size()- 2) {
        cerr << "Reached end of image ( after0xFF00 )." << endl;
        return -1;  
    }
    if(bytePos < data.size()-1 && data[bytePos] == 0xFF && data[bytePos + 1] != 0x00){
        cerr << "Reached another section." << endl;
        return -1;  
    }
    int bit = (data[bytePos] >> (7 - bitPos)) & 1;
    ++bitPos;
    if(bitPos == 8){
        bitPos = 0;
        ++bytePos;
    }
    return bit;
}
int getDClenth(vector<unsigned char> &data, int ColorID){
    int code = 0;
    int len = 0;
    while(len<16){
        ++len;
        code = code << 1;
        int bit = readBit(data);
        code += bit;
        if( HuffmanTable[0][SOStable[ColorID][0]].find(make_pair(len,code))
         != HuffmanTable[0][SOStable[ColorID][0]].end() ){
            return (int)HuffmanTable[0][SOStable[ColorID][0]][make_pair(len,code)];
         }
    }
    cerr << "Can't find the key in function 'getDClength'." << endl;
    return 0;
}
int readDCvalue(vector<unsigned char> &data,int ColorID){
    int length = getDClenth(data,ColorID);
    if(length == 0) return 0;
    int FirstBit = readBit(data);
    int value = 1;
    for(int i = 1; i < length ; ++i ){
        bool bit = readBit(data);
        value *= 2;
        if(FirstBit)
            value += bit;
        else
            value += !bit;
    }
    return FirstBit ? value : -value;
}
ACcoefficient readACvalue(vector<unsigned char> &data,int ColorID){
    int info = getACinfo(data,ColorID);
    unsigned char zeros = info >> 4;
    unsigned char length = info & 0x0F;
    if (info == 0x00) 
        return ACcoefficient{0,0,0};
    else if (info == 0xF0) 
        return ACcoefficient{16, 0, 0};
    int FirstBit = readBit(data);
    int value = 1;
    for(int i = 1; i < length ; ++i ){
        bool bit = readBit(data);
        value *= 2;
        if(FirstBit)
            value += bit;
        else
            value += !bit;
    }
    value = FirstBit ? value : -value;
    // cout <<"AC length,zeros,value: "<<(int)length<< "  "<<(int)zeros<<"  " << value<<endl;
    return ACcoefficient{zeros, length, value};
}
unsigned char getACinfo(vector<unsigned char> &data, int ColorID){
    int code = 0;
    int len = 0;
    while(len<16){
        ++len;
        code = code << 1;
        int bit = readBit(data);
        code += bit;
        if( HuffmanTable[1][SOStable[ColorID][1]].find(make_pair(len,code))
         != HuffmanTable[1][SOStable[ColorID][1]].end() ){
            return HuffmanTable[1][SOStable[ColorID][1]][make_pair(len,code)];
         }
    }
    cerr << "Can't find the key in function 'getACinfo'." << endl;
    return 0;
}
MCU readMCU(vector<unsigned char> &data){
    MCU curMCU;
    static int lastDC[4] = {}; // 開一個readMCU共用的DC陣列 index代表id，id0不用
    for ( int id = 1; id <= 3; ++id )
        for ( int vs = 0; vs < SOF0.component[id].verticalSampling; ++vs )
            for ( int hs = 0; hs < SOF0.component[id].horizontalSampling; ++hs){
                int DCdiff = readDCvalue(data,id);
                int DCvalue = DCdiff + lastDC[id];
                lastDC[id] = DCvalue;
                curMCU.mcu[id][vs][hs][0][0] = DCvalue;
                int curPOS = 1; // 在8*8的第一個位置(從零開始)，由左至右 由上至下
                while (curPOS<64) // 讀取此塊的63AC係數
                {
                    ACcoefficient curAC = readACvalue(data,id);
                    if ( curAC.length == 0 && curAC.zeros == 0 )  // 讀到0x00 ,EOB
                        break;
                    else if ( curAC.length == 0 && curAC.zeros == 16 ){ // 讀到0xF0 ,16個零
                        int curPOSfixed = curPOS;
                        for ( ; curPOS < curPOSfixed + 16; ++curPOS )   // 放入16個零
                            curMCU.mcu[id][vs][hs][curPOS/8][curPOS%8] = 0;
                    }
                    else{  // 正常讀到AC
                        int curPOSfixed = curPOS;
                        for ( ; curPOS < curPOSfixed + curAC.zeros; ++curPOS )   // 放入zeros個零
                            curMCU.mcu[id][vs][hs][curPOS/8][curPOS%8] = 0;
                        curMCU.mcu[id][vs][hs][curPOS/8][curPOS%8] = curAC.value;
                        ++curPOS;
                    }

                }
                while (curPOS<64){
                    curMCU.mcu[id][vs][hs][curPOS/8][curPOS%8] = 0;
                    ++curPOS;
                }
            }
    return curMCU;
}
void showMCU(MCU curMCU){
    printf("*************** mcu show ***********************\n");
    for (int id = 1; id <= 3; id++) {
        for (int h = 0; h < SOF0.component[id].verticalSampling; h++) {
            for (int w = 0; w < SOF0.component[id].horizontalSampling; w++) {
                printf("mcu id: %d, %d %d\n", id, h, w);
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        printf("%lf ", curMCU.mcu[id][h][w][i][j]);
                    }
                    printf("\n");
                }
            }
        }
    }
}
void inverseQuantify(MCU &curMCU){
    for ( int id = 1; id <= 3; ++id )
        for ( int vs = 0; vs < SOF0.component[id].verticalSampling; ++vs )
            for ( int hs = 0; hs < SOF0.component[id].horizontalSampling; ++hs)
                for ( int i = 0; i < 8; ++i )
                    for ( int j = 0; j < 8; ++j )
                        curMCU.mcu[id][vs][hs][i][j] *= QuantTable[SOF0.component[id].QuantTableID][i*8+j];
}
MCU inverseZigzag(MCU &curMCU){
    MCU afterZigzag;
    BLOCK zigzag = {
                    { 0,  1,  5,  6, 14, 15, 27, 28},
                    { 2,  4,  7, 13, 16, 26, 29, 42},
                    { 3,  8, 12, 17, 25, 30, 41, 43},
                    { 9, 11, 18, 24, 31, 40, 44, 53},
                    {10, 19, 23, 32, 39, 45, 52, 54},
                    {20, 22, 33, 38, 46, 51, 55, 60},
                    {21, 34, 37, 47, 50, 56, 59, 61},
                    {35, 36, 48, 49, 57, 58, 62, 63}
    };
    for ( int id = 1; id <= 3; ++id )
        for ( int vs = 0; vs < SOF0.component[id].verticalSampling; ++vs )
            for ( int hs = 0; hs < SOF0.component[id].horizontalSampling; ++hs)
                for ( int i = 0; i < 8; ++i )
                    for ( int j = 0; j < 8; ++j ){
                        int zigzagOrder = zigzag[i][j];
                        afterZigzag.mcu[id][vs][hs][i][j] = curMCU.mcu[id][vs][hs][zigzagOrder/8][zigzagOrder%8];
                    }
    return afterZigzag;
}
double c(int i) {
        static double x = 1.0/sqrt(2.0);
        if (i == 0) {
            return x;
        } else {
            return 1.0;
        }
}
void iDCT(MCU &curMCU){
    /**************初始化******************/
    for (int i = 0; i < 200; i++) {
        cos_cache[i] = cos(i * PI / 16.0);
    }

    for (int id = 1; id <= 3; id++) 
            for (int vs = 0; vs < SOF0.component[id].verticalSampling; ++vs) 
                for (int hs = 0; hs < SOF0.component[id].horizontalSampling; ++hs) {
                    double tmp[8][8] = {0};
                    double s[8][8] = {};
                    for (int j = 0; j < 8; j++) {
                        for (int x = 0; x < 8; x++) {
                            for (int y = 0; y < 8; y++) {
                                s[j][x] += c (y) * curMCU.mcu[id][vs][hs][x][y] * cos_cache[(j + j + 1) * y];
                            }
                            s[j][x] = s[j][x] / 2.0;
                        }
                    }
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            for (int x = 0; x < 8; x++) {
                                tmp[i][j] += c(x) * s[j][x] * cos_cache[(i + i + 1) * x];
                            }
                            tmp[i][j] = tmp[i][j] / 2.0;
                        }
                    }
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            curMCU.mcu[id][vs][hs][i][j] = tmp[i][j];
                        }
                    }
                }
}
vector<vector<RGB>> YCbCrtoRGB(MCU &curMCU){
    vector<vector<RGB>> rgb;
    /******************初始化大小********************/
    rgb.resize(maxVerticalSampling*8);
    for ( int i = 0; i < rgb.size(); ++i )
        rgb[i].resize(maxHorizontalSampling*8);
    /*****************變成一個MCU大小的RGB********************/

    for ( int i = 0; i < maxVerticalSampling * 8; ++i )
        for( int j = 0; j < maxHorizontalSampling * 8; ++j ){
            //進行轉換先升採樣再轉換色彩空間
        }
    
    return rgb;
}
