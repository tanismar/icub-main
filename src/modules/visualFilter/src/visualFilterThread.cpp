#include <iCub/visualFilterThread.h>
#include <stdio.h>

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

#define maxKernelSize 5

visualFilterThread::visualFilterThread() {
    redPlane=new ImageOf<PixelMono>;
    redPlane2=new ImageOf<PixelMono>;
    redPlane3=new ImageOf<PixelMono>;
    greenPlane=new ImageOf<PixelMono>;
    greenPlane2=new ImageOf<PixelMono>;
    greenPlane3=new ImageOf<PixelMono>;
    bluePlane=new ImageOf<PixelMono>;
    bluePlane2=new ImageOf<PixelMono>;
    bluePlane3=new ImageOf<PixelMono>;
    yellowPlane=new ImageOf<PixelMono>;
    yellowPlane2=new ImageOf<PixelMono>;
    inputExtImage=new ImageOf<PixelRgb>;

    redPlus=new ImageOf<PixelMono>;
    redMinus=new ImageOf<PixelMono>;
    greenPlus=new ImageOf<PixelMono>;
    greenMinus=new ImageOf<PixelMono>;
    bluePlus=new ImageOf<PixelMono>;
    yellowMinus=new ImageOf<PixelMono>;

    redGreen=new ImageOf<PixelMono>;
    greenRed=new ImageOf<PixelMono>;
    blueYellow=new ImageOf<PixelMono>;

    edges=new ImageOf<PixelMono>;
    redGreenEdgesHoriz=new ImageOf<PixelMono>;
    greenRedEdgesHoriz=new ImageOf<PixelMono>;
    blueYellowEdgesHoriz=new ImageOf<PixelMono>;
    redGreenEdgesVert=new ImageOf<PixelMono>;
    greenRedEdgesVert=new ImageOf<PixelMono>;
    blueYellowEdgesVert=new ImageOf<PixelMono>;
}


bool visualFilterThread::threadInit() {
    /* open ports  */ 

    if (!imagePortIn.open(getName("/image:i").c_str())) {
        cout <<": unable to open port "  << endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }

    if (!imagePortOut.open(getName("/image:o").c_str())) {
        cout << ": unable to open port "  << endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }

    if (!imagePortExt.open(getName("/imageExt:o").c_str())) {
        cout << ": unable to open port "  << endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }
    /* initialize variables and create data-structures if needed */
    return true;
}

void visualFilterThread::setName(string str){
    this->name=str;
    printf("name: %s", name.c_str());
}


std::string visualFilterThread::getName(const char* p){
    string str(name);
    str.append(p);
    //printf("name: %s", name.c_str());
    return str;
}

void visualFilterThread::run() {

   /* 
    * do some work ....
    * for example, convert the input image to a binary image using the threshold provided 
    */ 
   
   unsigned char value;

   while (isStopping() != true) { // the thread continues to run until isStopping() returns true
 
      
        
        inputImage = imagePortIn.read(true);

        if(inputImage!=NULL){
            resize(inputImage->width(),inputImage->height());      
          
            //extending logpolar input image
            extending();
            //extracting RGB and Y planes
            extractPlanes();
            //gaussing filtering of the of RGBY
            filtering();
            //colourOpponency map construction
            colourOpponency();
            //applying sobel operators on the colourOpponency maps and combining via maximisation of the 3 edges
            edgesExtract();
            //sending the edge image on the outport
                 
            if((edges!=0)&&(imagePortOut.getOutputCount())){
                imagePortOut.prepare() =*(edges);		
                imagePortOut.write();
            }
            if((inputExtImage!=0)&&(imagePortExt.getOutputCount())){
                imagePortExt.prepare() = *(inputExtImage);		
                imagePortExt.write();
            }

        }
   }//while 
}

void visualFilterThread::resize(int width_orig,int height_orig) {
    this->width_orig=width_orig;
    this->height_orig=height_orig;
    this->width=width_orig+2*maxKernelSize;
    this->height=height_orig+maxKernelSize;

    //resizing the ROI
    originalSrcsize.height=height_orig;
    originalSrcsize.width=width_orig;
    srcsize.width=width;
    srcsize.height=height;

    //resizing plane images
    edges->resize(width_orig, height_orig);
    inputExtImage->resize(width,height);
    redPlane->resize(width,height);
    redPlane2->resize(width,height);
    redPlane3->resize(width,height);
    greenPlane->resize(width,height);
    greenPlane2->resize(width,height);
    greenPlane3->resize(width,height);
    bluePlane->resize(width,height);
    bluePlane2->resize(width,height);
    bluePlane3->resize(width,height);
    yellowPlane->resize(width,height);
    yellowPlane2->resize(width,height);

    redPlus->resize(width,height);
    redMinus->resize(width,height);
    greenPlus->resize(width,height);
    greenMinus->resize(width,height);
    bluePlus->resize(width,height);
    yellowMinus->resize(width,height);

    redGreen->resize(width, height);
    greenRed->resize(width, height);
    blueYellow->resize(width, height);

    
    
    redGreenEdgesHoriz->resize(width, height);
    greenRedEdgesHoriz->resize(width, height);
    blueYellowEdgesHoriz->resize(width, height);
    redGreenEdgesVert->resize(width, height);
    greenRedEdgesVert->resize(width, height);
    blueYellowEdgesVert->resize(width, height);
}


void visualFilterThread::extending() {
    extender(inputImage,maxKernelSize);
}

ImageOf<PixelRgb>* visualFilterThread::extender(ImageOf<PixelRgb>* inputOrigImage,int maxSize) {
    //copy of the image 
    ippiCopy_8u_C3R(inputOrigImage->getRawImage(),inputOrigImage->getRowSize(),inputExtImage->getPixelAddress(maxSize,maxSize),inputExtImage->getRowSize(),originalSrcsize);    
    //memcpy of the horizontal fovea lines (rows) 
    int sizeBlock=width_orig/2;
    for( int i=0;i<maxSize;i++) {
        memcpy(inputExtImage->getPixelAddress(sizeBlock+maxSize,maxSize-1-i),inputExtImage->getPixelAddress(maxSize,maxSize+i),sizeBlock*sizeof(PixelRgb));
        memcpy(inputExtImage->getPixelAddress(maxSize,maxSize-1-i),inputExtImage->getPixelAddress(sizeBlock,maxSize+i),sizeBlock*sizeof(PixelRgb));
    }
    //copy of the block adiacent angular positions (columns)
    unsigned char* ptrDestRight;
    unsigned char* ptrOrigRight;
    unsigned char* ptrDestLeft;
    unsigned char* ptrOrigLeft;
    for(int row=0;row<height;row++) {
        ptrDestRight=inputExtImage->getPixelAddress(width-maxSize,row);
        ptrOrigRight=inputExtImage->getPixelAddress(maxSize,row);
        ptrDestLeft=inputExtImage->getPixelAddress(0,row);
        ptrOrigLeft=inputExtImage->getPixelAddress(width-maxSize,row);
        for(int i=0;i<maxSize;i++){
            //right block
            *ptrDestRight=*ptrOrigRight;
            ptrDestRight++;ptrOrigRight++;
            *ptrDestRight=*ptrOrigRight;
            ptrDestRight++;ptrOrigRight++;
            *ptrDestRight=*ptrOrigRight;
            ptrDestRight++;ptrOrigRight++;
            //left block
            *ptrDestLeft=*ptrOrigLeft;
            ptrDestLeft++;ptrOrigLeft++;
            *ptrDestLeft=*ptrOrigLeft;
            ptrDestLeft++;ptrOrigLeft++;
            *ptrDestLeft=*ptrOrigLeft;
            ptrDestLeft++;ptrOrigLeft++;
        }
    }
    return inputExtImage;
}
   
    
void visualFilterThread::extractPlanes() {
    Ipp8u* shift[3];
    int psb;
	shift[0]=redPlane->getRawImage(); 
	shift[1]=greenPlane->getRawImage();
	shift[2]=bluePlane->getRawImage();
    ippiCopy_8u_C3P3R(inputExtImage->getRawImage(),inputExtImage->getRowSize(),shift,redPlane->getRowSize(),srcsize);
    ippiAdd_8u_C1RSfs(redPlane->getRawImage(),redPlane->getRowSize(),greenPlane->getRawImage(),greenPlane->getRowSize(),yellowPlane->getRawImage(),yellowPlane->getRowSize(),srcsize,1);
}

    
void visualFilterThread::filtering() {
    IppiSize srcPlusSize = { 5, 5 };
    IppiSize srcMinusSize = { 7, 7 };
    Ipp32f srcMinus[7*7] ={0.0113, 0.0149, 0.0176, 0.0186, 0.0176, 0.0149, 0.0113,
    0.0149, 0.0197, 0.0233, 0.0246, 0.0233, 0.0197, 0.0149,
    0.0176, 0.0233, 0.0275, 0.0290, 0.0275, 0.0233, 0.0176,
    0.0186, 0.0246, 0.0290, 0.0307, 0.0290, 0.0246, 0.0186,
    0.0176, 0.0233, 0.0275, 0.0290, 0.0275, 0.0233, 0.0176,
    0.0149, 0.0197, 0.0233, 0.0246, 0.0233, 0.0197, 0.0149,
    0.0113, 0.0149, 0.0176, 0.0186, 0.0176, 0.0149, 0.0113};
    int divisor = 1;
    IppiPoint anchor={4,4};
    
    /*
    ippiFilter32f_8u_C1R(redPlane->getPixelAddress(maxKernelSize,maxKernelSize),redPlane->getRowSize(),redMinus->getPixelAddress(maxKernelSize,maxKernelSize),redMinus->getRowSize(),originalSrcsize,srcMinus,srcMinusSize,anchor);
    ippiFilter32f_8u_C1R(yellowPlane->getPixelAddress(maxKernelSize,maxKernelSize),yellowPlane->getRowSize(),yellowMinus->getPixelAddress(maxKernelSize,maxKernelSize),yellowMinus->getRowSize(),originalSrcsize,srcMinus,srcMinusSize,anchor);
    ippiFilter32f_8u_C1R(greenPlane->getPixelAddress(maxKernelSize,maxKernelSize),greenPlane->getRowSize(),greenMinus->getPixelAddress(maxKernelSize,maxKernelSize),greenMinus->getRowSize(),originalSrcsize,srcMinus,srcMinusSize,anchor);
    */
    ippiFilter32f_8u_C1R(redPlane->getRawImage(),redPlane->getRowSize(),redMinus->getRawImage(),redMinus->getRowSize(),srcsize,srcMinus,srcMinusSize,anchor);
    ippiFilter32f_8u_C1R(yellowPlane->getRawImage(),yellowPlane->getRowSize(),yellowMinus->getRawImage(),yellowMinus->getRowSize(),srcsize,srcMinus,srcMinusSize,anchor);
    ippiFilter32f_8u_C1R(greenPlane->getRawImage(),greenPlane->getRowSize(),greenMinus->getRawImage(),greenMinus->getRowSize(),srcsize,srcMinus,srcMinusSize,anchor);

    /*
    ippiFilterGauss_8u_C1R(bluePlane->getPixelAddress(maxKernelSize,maxKernelSize), bluePlane->getRowSize(),bluePlus->getPixelAddress(maxKernelSize,maxKernelSize),bluePlus->getRowSize(),srcsize,ippMskSize5x5);
    ippiFilterGauss_8u_C1R(redPlane->getPixelAddress(maxKernelSize,maxKernelSize), redPlane->getRowSize(),redPlus->getPixelAddress(maxKernelSize,maxKernelSize),redPlus->getRowSize(),originalSrcsize,ippMskSize5x5);
    ippiFilterGauss_8u_C1R(greenPlane->getPixelAddress(maxKernelSize,maxKernelSize), greenPlane->getRowSize(),greenPlus->getPixelAddress(maxKernelSize,maxKernelSize),greenPlus->getRowSize(),originalSrcsize,ippMskS
    */

    ippiFilterGauss_8u_C1R(bluePlane->getRawImage(), bluePlane->getRowSize(),bluePlus->getRawImage(),bluePlus->getRowSize(),srcsize,ippMskSize5x5);
    ippiFilterGauss_8u_C1R(redPlane->getRawImage(), redPlane->getRowSize(),redPlus->getRawImage(),redPlus->getRowSize(),srcsize,ippMskSize5x5);
    ippiFilterGauss_8u_C1R(greenPlane->getRawImage(), greenPlane->getRowSize(),greenPlus->getRawImage(),greenPlus->getRowSize(),srcsize,ippMskSize5x5);

    
}

void visualFilterThread::colourOpponency() {
    
    ippiRShiftC_8u_C1R(redPlus->getRawImage(),redPlane->getRowSize(),1,redPlane2->getRawImage(),redPlane2->getRowSize(),srcsize);
    ippiAddC_8u_C1RSfs(redPlane2->getRawImage(),redPlane2->getRowSize(),128,redPlane3->getRawImage(),redPlane3->getRowSize(),srcsize,0);
    ippiRShiftC_8u_C1R(redMinus->getRawImage(),redMinus->getRowSize(),1,redPlane2->getRawImage(),redPlane2->getRowSize(),srcsize);
    ippiRShiftC_8u_C1R(greenPlus->getRawImage(),greenPlus->getRowSize(),1,greenPlane2->getRawImage(),greenPlane2->getRowSize(),srcsize);
    ippiAddC_8u_C1RSfs(greenPlane2->getRawImage(),greenPlane2->getRowSize(),128,greenPlane3->getRawImage(),greenPlane3->getRowSize(),srcsize,0);
    ippiRShiftC_8u_C1R(greenMinus->getRawImage(),greenMinus->getRowSize(),1,greenPlane2->getRawImage(),greenPlane2->getRowSize(),srcsize);
    ippiRShiftC_8u_C1R(bluePlus->getRawImage(),bluePlus->getRowSize(),1,bluePlane2->getRawImage(),bluePlane2->getRowSize(),srcsize);
    ippiAddC_8u_C1RSfs(bluePlane2->getRawImage(),bluePlane2->getRowSize(),128,bluePlane3->getRawImage(),bluePlane3->getRowSize(),srcsize,0);
    ippiRShiftC_8u_C1R(yellowMinus->getRawImage(),yellowMinus->getRowSize(),1,yellowPlane2->getRawImage(),yellowPlane2->getRowSize(),srcsize);

    ippiSub_8u_C1RSfs(greenPlane2->getRawImage(),greenPlane2->getRowSize(),redPlane3->getRawImage(),redPlane3->getRowSize(),redGreen->getRawImage(),redGreen->getRowSize(),srcsize,0);
    ippiSub_8u_C1RSfs(redPlane2->getRawImage(),redPlane2->getRowSize(),greenPlane3->getRawImage(),greenPlane3->getRowSize(),greenRed->getRawImage(),greenRed->getRowSize(),srcsize,0);
    ippiSub_8u_C1RSfs(yellowPlane2->getRawImage(),yellowPlane2->getRowSize(),bluePlane3->getRawImage(),bluePlane3->getRowSize(),blueYellow->getRawImage(),blueYellow->getRowSize(),srcsize,0);
}

double max(double a,double b,double c){
    if(a>b)
        if(a>c)
            return a;
        else
            return c;
    else
        if(b>c)
            return b;
        else
            return c;
}

void visualFilterThread::edgesExtract() {
    //sobel operations
    ippiFilterSobelHoriz_8u_C1R(redGreen->getPixelAddress(maxKernelSize,maxKernelSize),redGreen->getRowSize(),redGreenEdgesHoriz->getRawImage(),redGreenEdgesHoriz->getRowSize(),originalSrcsize);
    ippiFilterSobelVert_8u_C1R(redGreen->getPixelAddress(maxKernelSize,maxKernelSize),redGreen->getRowSize(),redGreenEdgesVert->getRawImage(),redGreenEdgesVert->getRowSize(),originalSrcsize);
    ippiFilterSobelHoriz_8u_C1R(greenRed->getPixelAddress(maxKernelSize,maxKernelSize),greenRed->getRowSize(),greenRedEdgesHoriz->getRawImage(),greenRedEdgesHoriz->getRowSize(),originalSrcsize);
    ippiFilterSobelVert_8u_C1R(greenRed->getPixelAddress(maxKernelSize,maxKernelSize),greenRed->getRowSize(),greenRedEdgesVert->getRawImage(),greenRedEdgesVert->getRowSize(),originalSrcsize);
    ippiFilterSobelHoriz_8u_C1R(blueYellow->getPixelAddress(maxKernelSize,maxKernelSize),blueYellow->getRowSize(),blueYellowEdgesHoriz->getRawImage(),blueYellowEdgesHoriz->getRowSize(),originalSrcsize);
    ippiFilterSobelVert_8u_C1R(blueYellow->getPixelAddress(maxKernelSize,maxKernelSize),blueYellow->getRowSize(),blueYellowEdgesVert->getRawImage(),blueYellowEdgesVert->getRowSize(),originalSrcsize);
    //pointers
    unsigned char* prgh=redGreenEdgesHoriz->getRawImage();
    unsigned char* prgv=redGreenEdgesVert->getRawImage();
    unsigned char* pgrh=greenRedEdgesHoriz->getRawImage();
    unsigned char* pgrv=greenRedEdgesVert->getRawImage();
    unsigned char* pbyh=blueYellowEdgesHoriz->getRawImage();
    unsigned char* pbyv=blueYellowEdgesVert->getRawImage();
    unsigned char* pedges=edges->getRawImage();

    int rowsize=edges->getRowSize();
    int rowsize2=redGreenEdgesHoriz->getRowSize();

    //edges extraction
    for(int row=0;row<height_orig;row++) {
        for(int col=0;col<width_orig;col++) {
            double a=sqrt(pow((double)*prgh,2)+pow((double)*prgv,2));
            double b=sqrt(pow((double)*pgrh,2)+pow((double)*pgrv,2));
            double c=sqrt(pow((double)*pbyh,2)+pow((double)*pbyv,2));
            if(row<height_orig-2)
                *pedges=(unsigned char)ceil(max(a,b,c));
            else
                *pedges=0;
            
            prgh++;prgv++;
            pgrh++;pgrv++;
            pbyh++;pbyv++;
            pedges++;
            
        }
        for(int i=0;i<rowsize-width_orig;i++) {            
            pedges++;
        }
        for(int i=0;i<rowsize2-width_orig-maxKernelSize+maxKernelSize;i++){
            prgh++;prgv++;
            pgrh++;pgrv++;
            pbyh++;pbyv++;
        }
    }
}

void visualFilterThread::threadRelease() {
    /* for example, delete dynamically created data-structures */
    delete redPlane;
    delete redPlane2;
    delete redPlane3;
    delete greenPlane;
    delete greenPlane2;
    delete greenPlane3;
    delete bluePlane;
    delete bluePlane2;
    delete bluePlane3;
    delete yellowPlane;
    delete yellowPlane2;
    delete inputExtImage;

    delete redPlus;
    delete redMinus;
    delete greenPlus;
    delete greenMinus;
    delete bluePlus;
    delete yellowMinus;

    delete redGreen;
    delete greenRed;
    delete blueYellow;
    delete edges;
    
    delete redGreenEdgesHoriz;
    delete greenRedEdgesHoriz;
    delete blueYellowEdgesHoriz;
    delete redGreenEdgesVert;
    delete greenRedEdgesVert;
    delete blueYellowEdgesVert;
}

void visualFilterThread::onStop(){
   imagePortIn.interrupt();
   imagePortOut.interrupt();
   imagePortIn.close();
   imagePortOut.close();
   imagePortExt.close();
}


