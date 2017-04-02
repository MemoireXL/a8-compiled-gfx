#include "mainclass.h"
#include <QDebug>
#include <QImage>
#include <QSet>
#include <QMap>

#include <QTextStream>
#include <QFile>

QTextStream out(stdout);


MainClass::MainClass(QObject *parent) :
    QObject(parent)
{
    // get the instance of the main application
    app = QCoreApplication::instance();
    // setup everything here
    // create any global objects
    // setup debug and warning mode
}

int floor4(int x){
    return x&0xFFFC;
}

QMap<QRgb,int> colors;

int  pcolor(QRgb& color){
    int r=  colors[color];
    qDebug() << "r=" << r;
    return r;

}


void outpixel2(QImage& myImage,int byteIndex,int minx, int maxx,int y, QTextStream& afile){

    int curr_minx=byteIndex*4;
    int curr_maxx=(byteIndex+1)*4-1;
    int value=0;
    int maxvalue=0;

    qDebug()<<curr_minx <<curr_maxx << minx << maxx;
    qDebug()<<qMax(minx,curr_minx) << qMin(maxx,curr_maxx);

    int x;

    for (x=qMax(minx,curr_minx); x<=qMin(maxx,curr_maxx);x++){
        QRgb currentPixel = (myImage.pixel(2*x, y));
        value*=4;
        maxvalue*=4;

        if (qAlpha(currentPixel) != 0) {
            value+=pcolor(currentPixel);
            maxvalue+=3;
        }
    }

    for (; x<=curr_maxx;x++){
        value*=4;
        maxvalue*=4;

    }

    int mask=255-maxvalue;


    // maxvalue = 255-mask (1) ou de facon equivalente max = 255-maxvalue

    // or par definition
    // value =< maxvalue // some 0 instead of 1
    //
    // d'ou value =< 255-mask (1)
    // et donc
    // value + mask =< 255

    // minvalue = 0

    // donc

    // mask =< value+mask =< maxvalue+mask

    // if (value==mask)
    // mask =< mask+mask =< maxvalue + mask

    // mask+mask =< 255

    // screen value = ((screen value) & mask) | value

    if (mask==0){
        // screen value = value
        afile << "\tLDA #" << value << endl;
        afile << "\tSTA ($80),Y" << endl;
    } else {
        // mask != 0
        if (mask==255){
            // value + mask =< 255
            // hence value=0;
            // Nothing to do
        } else {
            // mask < 255
            if (value==0){
                afile << "\tLDA ($80),Y" << endl;
                afile << "\tAND #" << mask << endl;
                afile << "\tSTA ($80),Y" << endl;
            } else if (mask+value==255){
                // value = maxvalue
                // -> no '0' to put
                afile << "\tLDA #" << value << endl;
                afile << "\tORA #" << value << endl;
                afile << "\tSTA ($80),Y" << endl;
            } else {
                // mask+value < 255
                afile << "\tLDA ($80),Y" << endl;
                afile << "\tAND #" << mask << endl;
                afile << "\tORA #" << value << endl;
                afile << "\tSTA ($80),Y" << endl;
            }
        }
    }

    afile << "\tINY" << endl;
}

void startLine(int index, int y , QTextStream& afile){
    afile << "\tldy#"<< index <<"\t;line " << y<< endl;

}
void endLine( QTextStream& afile){
    afile << "\tjsr add_y" << endl;

}

QString image="32x19";

// 10ms after the application starts this method will run
// all QT messaging is running at this point so threads, signals and slots
// will all work as expected.
void MainClass::run()
{

    QFile data("/home/teddy/output_"+image+".asm");
    data.open(QFile::WriteOnly | QFile::Truncate);
    QTextStream asm_file(&data);


    QImage myImage;
    myImage.load("/home/teddy/Documents/"+image+".png");

    int minYTransparent = myImage.height();
    int maxYTransparent = -1;

    for (int x = 0; x <  myImage.width()/2; x++) {
        for (int y = 0; y <  myImage.height(); y++) {
            QRgb currentPixel = (myImage.pixel(x*2, y));

            if (qAlpha(currentPixel) != 0) {

                if (!colors.contains(currentPixel)){
                    colors[currentPixel]=colors.size();
                }

                // Opaque pixel;
                minYTransparent = qMin(minYTransparent,y);
                maxYTransparent = qMax(maxYTransparent,y);
            }
        }
    }


    qDebug() << "Opaque zone : " <<  minYTransparent <<  maxYTransparent << colors.size() ;
    //   minXTransparent=17;

    for (int y = minYTransparent; y <=  maxYTransparent; y++) {
        int minXTransparent = myImage.width()/2;
        int maxXTransparent = -1;


        for (int x = 0; x <  myImage.width()/2; x++) {
            QRgb currentPixel = (myImage.pixel(2*x, y));
            if (qAlpha(currentPixel) != 0) {
                // Opaque pixel;
                minXTransparent = qMin(minXTransparent,x);
                maxXTransparent = qMax(maxXTransparent,x);
            }
        }
        startLine(floor4(minXTransparent)/4,y,asm_file);


        for (int byteIndex=floor4(minXTransparent)/4; byteIndex<=floor4(maxXTransparent)/4; byteIndex++){
            outpixel2( myImage, byteIndex,minXTransparent,maxXTransparent,y,asm_file);
        }

        endLine(asm_file);
    }


    quit();

}

// call this routine to quit the application
void MainClass::quit()
{
    // you can do some cleanup here
    // then do emit finished to signal CoreApplication to quit
    emit finished();
}

// shortly after quit is called the CoreApplication will signal this routine
// this is a good place to delete any objects that were created in the
// constructor and/or to stop any threads
void MainClass::aboutToQuitApp()
{
    // stop threads
    // sleep(1);   // wait for threads to stop.
    // delete any objects
}
