/**
 * File: Demo.cpp
 * Date: November 2011
 * Author: Dorian Galvez-Lopez
 * Description: demo application of DBoW2
 * License: see the LICENSE.txt file
 */

#include <iostream>
#include <vector>

// DBoW2
#include "DBoW2.h" // defines OrbVocabulary and OrbDatabase

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/xfeatures2d.hpp"

using namespace DBoW2;
using namespace std;
using namespace cv;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void loadFeatures(vector<vector<cv::Mat > > &features ,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list);
void changeStructure(const cv::Mat &plain, vector<cv::Mat> &out);
void testVocCreation(const vector<vector<cv::Mat > > &features);
void testDatabase(const vector<vector<cv::Mat > > &features,const std::string db_path,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// number of training images
//const int NIMAGES = 4;
const int NIMAGES = 3000;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void wait()
{
  cout << endl << "Press enter to continue" << endl;
  getchar();
}

// ----------------------------------------------------------------------------

int main()
{
 try{
  vector<vector<cv::Mat > > features;

  vector<vector<cv::KeyPoint> > kp_list;
  vector<cv::Mat> kdesp_list;
  loadFeatures(features,kp_list,kdesp_list);  //image to features.

  testVocCreation(features); //Do not create any voc.Use a pretrained one.

  //wait();

  testDatabase(features,"small_voc.yml.gz",kp_list,kdesp_list);//do test.
 }
 catch (...) {
    // this executes if f() throws std::string or int or any other unrelated type
   cout<<"Caught error!"<<endl;
 }

  return 0;
}

// ----------------------------------------------------------------------------

void loadFeatures(vector<vector<cv::Mat > > &features ,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list)
{
  features.clear();
  features.reserve(NIMAGES);

  cv::Ptr<cv::ORB> orb = cv::ORB::create();

  cout << "Extracting ORB features..." << endl;
  for(int i = 0; i < NIMAGES; ++i)
  {
    stringstream ss;
    ss << "images/image" << i << ".png";

    cv::Mat image = cv::imread(ss.str(), 0);
    cv::Mat mask;
    vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;

    orb->detectAndCompute(image, mask, keypoints, descriptors);

    features.push_back(vector<cv::Mat >());
    changeStructure(descriptors, features.back());

    
    kp_list.push_back(keypoints);
    kdesp_list.push_back(descriptors);
  }
}

// ----------------------------------------------------------------------------

void changeStructure(const cv::Mat &plain, vector<cv::Mat> &out)
{
  out.resize(plain.rows);

  for(int i = 0; i < plain.rows; ++i)
  {
    out[i] = plain.row(i);
  }
}

// ----------------------------------------------------------------------------

void testVocCreation(const vector<vector<cv::Mat > > &features)
{
  // branching factor and depth levels 
  const int k = 9;
  const int L = 3;
  const WeightingType weight = TF_IDF;
  const ScoringType score = L1_NORM;

  OrbVocabulary voc(k, L, weight, score);

  cout << "Creating a small " << k << "^" << L << " vocabulary..." << endl;
  voc.create(features);
  cout << "... done!" << endl;

  cout << "Vocabulary information: " << endl
  << voc << endl << endl;

  // lets do something with this vocabulary
  cout << "Matching images against themselves (0 low, 1 high): " << endl;
  BowVector v1, v2;
  for(int i = 0; i < NIMAGES; i++)
  {
    voc.transform(features[i], v1);
    for(int j = i-4; j < i+4; j++)
    {
      if(j<0)
      {
        continue;
      }
      if(j>=NIMAGES)
      {
        continue;

      }
      voc.transform(features[i], v2);
      
      double score = voc.score(v1, v2);
      cout << "Image " << i << " vs Image " << j << ": " << score << endl;
    }
  }

  // save the vocabulary to disk
  cout << endl << "Saving vocabulary..." << endl;
  voc.save("small_voc.yml.gz");
  cout << "Done" << endl;
}



void saveImagePair(int id1,int id2,int &save_index,std::vector<DMatch>& good_matches,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list);

// ----------------------------------------------------------------------------

bool match_2_images_flann(int index1,int index2,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list,int &save_index)
{
  using namespace cv::xfeatures2d;
  FlannBasedMatcher matcher = FlannBasedMatcher(makePtr<flann::LshIndexParams>(12,20,2));
  std::vector< DMatch > matches;
  matcher.match( kdesp_list[index1], kdesp_list[index2], matches );

  cout<<"Matched!"<<endl;


  double max_dist = 0; double min_dist = 100;
  for( int i = 0; i < matches.size(); i++ )
  {
    double dist = matches[i].distance;
    if( dist < min_dist ) min_dist = dist;
    if( dist > max_dist ) max_dist = dist;
  }

  std::vector< DMatch > good_matches;

  for( int i = 0; i < matches.size(); i++ )
  {
    if( matches[i].distance <= 2*min_dist )
    {
      good_matches.push_back( matches[i]); 
    }
  }

  std::vector<cv::Point2f> match_points1;
  std::vector<cv::Point2f> match_points2;

  cout<<"Good matches count:"<<good_matches.size()<<"."<<endl;
  if(good_matches.size()<8)
  {
    cout<<"Good matches count:"<<good_matches.size()<<"< 8,MATCH FAILED."<<endl;
    return false;
  }
  for( size_t i = 0; i < good_matches.size(); i++ )
  {
    //-- Get the keypoints from the good matches
    //cout<<"Step1_"<<i<<endl;
    //cout<<"kp_list size:"<<kp_list.size()<<endl;
    //cout<<"kplist index:"<<index1<<endl;
    //auto a = kp_list[index1];
    //cout<<"2";
    //auto b = good_matches[i];
    //cout <<"3";
    //auto c = good_matches[i].queryIdx;
    //cout<<4;
    match_points1.push_back( kp_list[index1][ good_matches[i].queryIdx ].pt );
    //cout<<"Step2_"<<i<<endl;

    //auto d = kp_list[index2];
    //cout<<5;
    //auto e = good_matches[i].trainIdx;
    //cout<<6;
    match_points2.push_back( kp_list[index2][ good_matches[i].trainIdx ].pt );
  }
  saveImagePair(index1,index2,save_index,good_matches,kp_list,kdesp_list);
  Mat fundamental_matrix = findFundamentalMat(match_points1, match_points2, FM_RANSAC, 3, 0.99);
  cout<<"MATCH SUCCESS."<<endl;
  return true;

}

void saveImagePair(int id1,int id2,int &save_index,std::vector<DMatch>& good_matches,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list)
{
    stringstream ss;
    ss << "images/image" << id1 << ".png"; 
    cv::Mat image = cv::imread(ss.str(), 0);
    
    stringstream ss2;
    ss2<< "images/image"<<id2<<".png";
    cv::Mat image2 = cv::imread(ss2.str(),0);

//
    cv::Mat merged_img;
//    cv::hconcat(image,image2,merged_img);

//use cv::drawMatches replace hconcat.
    
    cv::drawMatches(image,kp_list[id1],image2,kp_list[id2],good_matches,merged_img);
    stringstream output_ss;

    output_ss<<"loops/image"<<save_index<<".png";
    cv::imwrite(output_ss.str(),merged_img);
    save_index++;
}


void testDatabase(const vector<vector<cv::Mat > > &features,const std::string db_path,vector<vector<cv::KeyPoint> > &kp_list,vector<cv::Mat> &kdesp_list)
{
  cout << "Creating a small database..." << endl;

  // load the vocabulary from disk
  OrbVocabulary voc(db_path.c_str());
  
  OrbDatabase db(voc, false, 0); // false = do not use direct index
  // (so ignore the last param)
  // The direct index is useful if we want to retrieve the features that 
  // belong to some vocabulary node.
  // db creates a copy of the vocabulary, we may get rid of "voc" now

  // add images to the database


  db.add(features[0]);
  int loop_id = 0;
  for(int i = 0; i < NIMAGES; i++)
  {
  

  //cout << "... done!" << endl;

  //cout << "Database information: " << endl << db << endl;

    // and query the database
    cout << "Querying the database: " << endl;

    QueryResults ret;
    db.query(features[i], ret, 4);

    // ret[0] is always the same image in this case, because we added it to the 
    // database. ret[1] is the second best match.

    cout << "Searching for Image " << i << ". " << ret << endl;
    //ret[0].Score;ret[0].Id
    if (ret[0].Score>0.65)
    {
      if (ret[0].Id<i-10)
      {
        if (match_2_images_flann(i,ret[0].Id,kp_list,kdesp_list,loop_id)) // check if this loop candidate satisfies Epipolar Geometry constrain.
        {
          cout<<"Loop between ["<<i<<"\t"<<ret[0].Id<<"]"<<endl;
        }
      }
    }
    else
    {
      db.add(features[i]);
    }
  }

  cout << endl;

  // we can save the database. The created file includes the vocabulary
  // and the entries added
  cout << "Saving database..." << endl;
  db.save("small_db.yml.gz");
  cout << "... done!" << endl;
  
  // once saved, we can load it again  
  cout << "Retrieving database once again..." << endl;
  OrbDatabase db2("small_db.yml.gz");
  cout << "... done! This is: " << endl << db2 << endl;
}

// ----------------------------------------------------------------------------


