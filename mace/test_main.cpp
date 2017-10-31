#include "opencv2/ts.hpp"
//#include "opencv2/objdetect.hpp"
#include "mace.h"



using namespace cv;
using namespace testing;
using namespace std;

#define TESTSET_NAMES testing::Values("david","dudek","faceocc2")

const string TRACKING_DIR = "tracking";
const string FOLDER_IMG = "data";
const string FOLDER_OMIT_INIT = "initOmit";


class MaceTest
{
 public:

  MaceTest(string _video, int salt, bool multi);
  virtual ~MaceTest() {}
  virtual void run();

 protected:
  vector<Rect> boxes(const string &fn);
  vector<Mat> samples(const string &name, int N,int off=0);
  int found(const string &vid);

  Ptr<MACE> mace;
  //CascadeClassifier cascade;
  string video;
  string vidA, vidB;
 // int startFrame;
  int nSampsTest;
  int nSampsTrain;
  int nStep;
  int salt;
};

MaceTest::MaceTest(string _video, int salt, bool multi)
{
	  int Z = 64; // window size
    if (multi)
        mace = MACE::createSampler(Z,vector<Rect2d>{
            Rect2d(0,0,1,1),           // whole image
            Rect2d(0.25,0.5,0.5,0.5),  // bottom center(mouth)
            Rect2d(0,0,0.5,0.5),       // top left (eye)
            Rect2d(0.5,0,0.5,0.5)      // top right (eye)
        });
    else
        mace = MACE::create(Z);

    video = _video;
    if (video=="david") { vidA="dudek"; vidB="faceocc2"; }
    if (video=="dudek") { vidA="david"; vidB="faceocc2"; }
    if (video=="faceocc2") { vidA="dudek"; vidB="david"; }
  //  cerr << "created test for " << video << endl;

  //  startFrame = 600;
    nStep = 2;
    nSampsTest = 10;
    nSampsTrain = 50;
    this->salt = salt;

    //cascade.load(cvtest::TS::ptr()->get_data_path() + "/cascadeandhog/cascades/haarcascade_frontalface_alt.xml");
    //CV_Assert(!cascade.empty());
}

vector<Rect> MaceTest::boxes(const string &fn)
{
  ifstream in(fn);
  int x,y,w,h;
  char sep;
  vector<Rect> boxes;
  while (in.good() && (in >> x >> sep >> y >> sep >> w >> sep >> h))
  {
    boxes.push_back( Rect(x,y,w,h) );
  }
  return boxes;
}

void MaceTest::run() {
//  cerr << "running test for " << video << endl;
  vector<Mat> sam_train = samples(video, nSampsTrain, 0);
  if (salt)
    mace->salt(salt);
  mace->train(sam_train);
  int self_ok = found(video);
  if (salt)
    mace->salt(~salt);
  int false_A = found(vidA);
  int false_B = found(vidB);
  ASSERT_GE(self_ok, nSampsTest/2);  // it may miss positives
  ASSERT_EQ(false_A, 0);  // but absolutely no false positives allowed.
  ASSERT_EQ(false_B, 0);
}

int MaceTest::found(const string &vid) {
  vector<Mat> sam_test = samples(vid, nSampsTest, (1+2*nSampsTrain));
  int hits = 0;
  for (size_t i=0; i<sam_test.size(); i++) {
    hits += mace->same(sam_test[i]);
  }
  cout << vid << " " << hits << " " << sam_test.size() << endl;
  return hits;
}

vector<Mat> MaceTest::samples(const string &name, int N, int off) {
  string folder = cvtest::TS::ptr()->get_data_path() + TRACKING_DIR + "/" + name;
  string vid  = folder + "/" + FOLDER_IMG + "/" + name + ".webm";
  string anno = folder + "/gt.txt";
  vector<Rect> bb = boxes(anno);
//  cerr << bb.size() << " boxes in " << anno << endl;
  int startFrame = (name=="david") ? 300 : 1;
  VideoCapture c;
  bool ok = c.open(vid);
//  cerr << ok << " " << c.get(CAP_PROP_FRAME_COUNT) << " frames in " << vid << endl;
  vector<Mat> samps;
  while (samps.size() < N) {
    int frameNo = startFrame + off;
    c.set(CAP_PROP_POS_FRAMES, frameNo);
    Mat frame;
    c >> frame;
    Rect r = bb[off];
   // cout << frameNo << " " << N << " " << samps.size() << " " << off << r << endl;
    off += nStep;
    samps.push_back(frame(r));
    rectangle(frame,r,Scalar(200,0,0));
    imshow("W", frame);
    waitKey(30);
  }
  c.release();
  return samps;
}

//[TESTDATA]
PARAM_TEST_CASE(MACE_, string)
{
  string dataset;
  virtual void SetUp()
  {
    dataset = GET_PARAM(0);
  }
};


TEST_P(MACE_, unsalted_single)
{
  MaceTest test(dataset, 0, false); test.run();
}
TEST_P(MACE_, unsalted_multi)
{
  MaceTest test(dataset, 0, true); test.run();
}
TEST_P(MACE_, salted_single)
{
  MaceTest test(dataset, 627364238, false); test.run();
}
TEST_P(MACE_, salted_multi)
{
  MaceTest test(dataset, 627364238, true); test.run();
}


INSTANTIATE_TEST_CASE_P( Face, MACE_, TESTSET_NAMES);

CV_TEST_MAIN("cv")
