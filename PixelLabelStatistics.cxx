#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

// Resample a series
//   Usage: PixelLabelStatistics inputLabel inputImage
//                        
//
// Description:
// Calculate certain statistics measures; this is promarily for
// post-processing of DCE parameter maps and quantification

#include "itkOrientedImage.h"
#include "itkImageFileReader.h"
#include "itkImageRegionConstIteratorWithIndex.h"

#include "PixelLabelStatisticsCLP.h"

#include <string>
#include <sstream>
#include <algorithm>


typedef itk::OrientedImage<float, 3> ImageType;
typedef itk::ImageFileReader<ImageType> ReaderType;
typedef itk::ImageRegionConstIteratorWithIndex<ImageType> IterType;

float GetMean(std::vector<float>);
float GetIQR(std::vector<float>);
float GetTop10Mean(std::vector<float>, float);
float GetBottom10Mean(std::vector<float>, float);
float GetMedian(std::vector<float>);

void PrintListStatistics(std::vector<float>);

int main(int argc, char* argv[]){
  PARSE_ARGS;

  //std::cout << 3dd"Input volume: " << InputVolume 
  //  << ", label: " << InputLabel << std::endl << std::endl;

  ImageType::Pointer mask = NULL;
  ImageType::Pointer rsq = NULL;
  ReaderType::Pointer volReader = ReaderType::New();
  volReader->SetFileName(InputVolume.c_str());
  volReader->Update();

  ReaderType::Pointer labReader = ReaderType::New();
  labReader->SetFileName(InputLabel.c_str());
  labReader->Update();

  ImageType::Pointer label = labReader->GetOutput();
  ImageType::Pointer image = volReader->GetOutput();

  if(RsqVolume != ""){
    //std::cout << "Rsq image: " << RsqVolume << std::endl;  
    ReaderType::Pointer rsqReader = ReaderType::New();
    rsqReader->SetFileName(RsqVolume.c_str());
    rsqReader->Update();
    rsq = rsqReader->GetOutput();
  } else {
    rsq = label;
  }

  std::vector<float> measurements;
  unsigned outsidePts = 0, totalPts = 0, 
           totalValidPts = 0, outsideMaskPts = 0;
  ImageType::PointType pt;
  ImageType::IndexType id;

  IterType it(label, label->GetBufferedRegion());
  IterType itRsq(rsq, rsq->GetBufferedRegion());
  IterType itMap(image, image->GetBufferedRegion());
  itRsq.GoToBegin();
  it.GoToBegin();
  itMap.GoToBegin();
  //std::cout << "Label of interest: " << LabelOfInterest << std::endl;
  float rsqMin = 10, rsqMax = -10, rsqMean = 0;
  std::cout << "Output format: <case_id> <label_value> <pixel_value>" << std::endl;
  while(!it.IsAtEnd()){
    if(it.Get() == LabelOfInterest || (it.Get() && LabelOfInterest == -1) ) {
      ImageType::PointType pt;
      ImageType::IndexType idx;
      idx = it.GetIndex();
      label->TransformIndexToPhysicalPoint(idx, pt);
      if(RsqVolume != ""){
        rsqMin = fmin(itRsq.Get(), rsqMin);
        rsqMax = fmax(itRsq.Get(), rsqMax);
        rsqMean += itRsq.Get();
        
        if(itRsq.Get() >= MinRsq){
          ++totalValidPts;
          measurements.push_back(itMap.Get());
        if(!OutputType){
          std::cout << CaseID << " " << it.Get() << " " << itMap.Get() << std::endl;
        } else {
          std::cout << CaseID << " " << it.Get() << " " << itMap.Get() << 
            " " << pt[0] << " " << pt[1] << " " << pt[2] << std::endl;
        }

        } 

      } else {
        ++totalValidPts;
        measurements.push_back(itMap.Get());
        if(!OutputType){
          std::cout << CaseID << " " << it.Get() << " " << itMap.Get() << std::endl;
        } else {
          std::cout << CaseID << " " << it.Get() << " " << itMap.Get() << 
            " " << pt[0] << " " << pt[1] << " " << pt[2] << std::endl;
        }
      }
      ++totalPts;
     /*
      id = it.GetIndex();
      label->TransformIndexToPhysicalPoint(id, pt);
      if(image->TransformPhysicalPointToIndex(pt, id)){
        if(mask){
          if(mask->GetPixel(id)){
            measurements.push_back(image->GetPixel(id));
          } else {
            outsideMaskPts++;
          }
        } else {
            measurements.push_back(image->GetPixel(id));
        }
      } else {
        outsidePts++;
      }
      */
    }
    ++it;++itMap;++itRsq;
  }

  return EXIT_SUCCESS;
}

void PrintListStatistics(std::vector<float> v){
  float mean = 0, std = 0, min = 100., max = -100.;
  for(std::vector<float>::const_iterator vI=v.begin();vI!=v.end();++vI){
    mean += *vI;
    min = fmin(min, *vI);
    max = fmax(max, *vI);
  }
  mean /= (float) v.size();

  for(std::vector<float>::const_iterator vI=v.begin();vI!=v.end();++vI){
    std += (*vI-mean)*(*vI-mean);
  }
  std = sqrt(std/(float) v.size());
  std::cout << 
    " Min: " << min << std::endl <<
    " Max: " << max << std::endl << 
    " Mean: " << mean << std::endl << 
    " STD: " << std << std::endl << std::endl;
}

float GetMean(std::vector<float> v){
  float mean = 0;
  for(std::vector<float>::const_iterator vI=v.begin();vI!=v.end();++vI){
    mean += *vI;
  }
  mean /= (float) v.size();
  return mean;
};


float GetIQR(std::vector<float> v){
  std::sort(v.begin(), v.end());
  return v[v.size()*3/4]-v[v.size()/4];
}

float GetTop10Mean(std::vector<float> v, float Threshold){
  std::sort(v.begin(), v.end());
  unsigned firstId, lastId;

  firstId = (unsigned) ((1.-Threshold) * (float)v.size());
  lastId = v.size()-1;
  float mean = 0;
  for(unsigned i=firstId;i<=lastId;i++)
     mean += v[i];
  mean /= lastId-firstId;
  return mean;
}


float GetBottom10Mean(std::vector<float> v, float Threshold){
  std::sort(v.begin(), v.end());
  unsigned firstId, lastId;

  firstId = 0;
  lastId = (unsigned) (Threshold * (float)v.size());
  float mean = 0;
  for(unsigned i=firstId;i<=lastId;i++)
    mean += v[i];
  mean /= lastId-firstId;
  return mean;
}

float GetMedian(std::vector<float> v){
  std::sort(v.begin(), v.end());

  return v[v.size()*.5];
}
