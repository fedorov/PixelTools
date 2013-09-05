#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

// Resample a series
//   Usage: PixelLabelStatistics caseID labelVolume mapVolume1 ... mapVolumeN
//                        
//
// Description:
// Calculate certain statistics measures; this is promarily for
// post-processing of DCE parameter maps and quantification
// 
// NOTE: the input volumes do not need to have the same pixel raster, statistics
// is extracted based on the physical coordinates of non-zero pixels in the label 
//volume. The input maps do not need to be resampled to the same reference.

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageRegionConstIteratorWithIndex.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>


typedef itk::Image<float, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::ImageRegionConstIteratorWithIndex<ImageType> IterType;

void PrintListStatistics(std::vector<float>);

struct Summary {
  float mean;
  float sd;
  float count;
  std::vector<float> values;

  Summary(){
    mean = 0;
    sd = 0;
    count = 0;
    values.clear();
  }
};

int main(int argc, char* argv[]){

  if(argc<4){
    std::cerr << "not enough arguments" << std::endl;
    return -1;
  }
  char* caseID = argv[1];
  char* labelFileName = argv[2];
  
  std::vector<ImageType::Pointer> images;
  for(int i=2;i<argc;i++){
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(argv[i]);
    reader->Update();
    images.push_back(reader->GetOutput());
  }

  IterType itLabel(images[0], images[0]->GetBufferedRegion());
  for(itLabel.GoToBegin();!itLabel.IsAtEnd();++itLabel){
    if(!itLabel.Get())
      continue;
    ImageType::IndexType idx = itLabel.GetIndex();
    ImageType::PointType pt;
    images[0]->TransformIndexToPhysicalPoint(idx,pt);
    std::cout << caseID << "," << itLabel.Get() << "," << 
      pt[0] << "," << pt[1] << "," << pt[2] << ",";    
    for(int i=1;i<images.size();i++){
      ImageType::IndexType idxImage;
      if(!images[i]->TransformPhysicalPointToIndex(pt, idxImage)){
        std::cerr << "Attempt to sample a point outside image " << i << ": failed" << std::endl;
        return -1;
      }
      std::cout << images[i]->GetPixel(idxImage);
      if(i<images.size()-1)
        std::cout << ",";
    }
    std::cout << std::endl;
  }
  return EXIT_SUCCESS;
}
