// @(#)root/test:$Id$
// Author: Rene Brun   19/01/97

////////////////////////////////////////////////////////////////////////
//
//             A simple example with a ROOT tree
//             =================================
//
//  This program creates :
//    - a ROOT file
//    - a tree
//  Additional arguments can be passed to the program to control the flow
//  of execution. (see comments describing the arguments in the code).
//      Event  nevent comp split fill
//  All arguments are optional: Default is
//      Event  400      1    1     1
//
//  In this example, the tree consists of one single "super branch"
//  The statement ***tree->Branch("event", event, 64000,split);*** below
//  will parse the structure described in Event.h and will make
//  a new branch for each data member of the class if split is set to 1.
//    - 5 branches corresponding to the basic types fNtrack,fNseg,fNvertex
//           ,fFlag and fTemperature.
//    - 3 branches corresponding to the members of the subobject EventHeader.
//    - one branch for each data member of the class Track of TClonesArray.
//    - one branch for the object fH (histogram of class TH1F).
//
//  if split = 0 only one single branch is created and the complete event
//  is serialized in one single buffer.
//  if comp = 0 no compression at all.
//  if comp = 1 event is compressed.
//  if comp = 2 same as 1. In addition branches with floats in the TClonesArray
//                         are also compressed.
//  The 4th argument fill can be set to 0 if one wants to time
//     the percentage of time spent in creating the event structure and
//     not write the event in the file.
//  In this example, one loops over nevent events.
//  The branch "event" is created at the first event.
//  The branch address is set for all other events.
//  For each event, the event header is filled and ntrack tracks
//  are generated and added to the TClonesArray list.
//  For each event the event histogram is saved as well as the list
//  of all tracks.
//
//  The number of events can be given as the first argument to the program.
//  By default 400 events are generated.
//  The compression option can be activated/deactivated via the second argument.
//
//   ---Running/Linking instructions----
//  This program consists of the following files and procedures.
//    - Event.h event class description
//    - Event.C event class implementation
//    - MainEvent.C the main program to demo this class might be used (this file)
//    - EventCint.C  the CINT dictionary for the event and Track classes
//        this file is automatically generated by rootcint (see Makefile),
//        when the class definition in Event.h is modified.
//
//   ---Analyzing the Event.root file with the interactive root
//        example of a simple session
//   Root > TFile f("Event.root")
//   Root > T.Draw("fNtrack")   //histogram the number of tracks per event
//   Root > T.Draw("fPx")       //histogram fPx for all tracks in all events
//   Root > T.Draw("fXfirst:fYfirst","fNtrack>600")
//                              //scatter-plot for x versus y of first point of each track
//   Root > T.Draw("fH.GetRMS()")  //histogram of the RMS of the event histogram
//
//   Look also in the same directory at the following macros:
//     - eventa.C  an example how to read the tree
//     - eventb.C  how to read events conditionally
//
////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string>
#include <sstream>

#include "Compression.h"
#include "TROOT.h"
#include "TFile.h"
#include "TRandom.h"
#include "TTree.h"
#include "TBranch.h"
#include "TClonesArray.h"
#include "TStopwatch.h"
#include "TClass.h"
#include "Event.h"
#include "TSocket.h"
#include "TObjString.h"
#include "TMessage.h"
#include "TBuffer.h"
#include "RConfigure.h"

#ifdef R__HAS_DEFAULT_LZ4
constexpr int expectedcomplevel = 4;
#else
constexpr int expectedcomplevel = 1;
#endif


// define a test class because we need to use some protected methods in the test
class TestTMessage : public TMessage {
public:
   TestTMessage(void *buf, Int_t bufsize) : TMessage(buf, bufsize) { }
   TestTMessage(UInt_t what = kMESS_ANY, Int_t bufsiz = TBuffer::kInitialSize) :
      TMessage(what, bufsiz) { }
   void SetLength() { TMessage::SetLength(); }
};

//______________________________________________________________________________
int main(int argc, char **argv)
{
   TROOT simple("simple","Example of creation of a tree");

   gRandom->SetSeed(42); // make tests reproducible

   TMessage * message = new TMessage();
   int testValue = -2;
   message->SetCompressionSettings(testValue);
   if (message->GetCompressionSettings() != -1) exit(130);
   if (message->GetCompressionAlgorithm() != -1) exit(131);
   if (message->GetCompressionLevel() != -1) exit(132);

   testValue = -1;
   message->SetCompressionSettings(testValue);
   if (message->GetCompressionSettings() != -1) exit(133);
   if (message->GetCompressionAlgorithm() != -1) exit(134);
   if (message->GetCompressionLevel() != -1) exit(135);

   testValue = 0;
   message->SetCompressionSettings(testValue);
   if (message->GetCompressionSettings() != 0) exit(136);
   if (message->GetCompressionAlgorithm() != 0) exit(137);
   if (message->GetCompressionLevel() != 0) exit(138);

   testValue = 1112;
   message->SetCompressionSettings(testValue);
   if (message->GetCompressionSettings() != 1112) exit(139);
   if (message->GetCompressionAlgorithm() != 11) exit(140);
   if (message->GetCompressionLevel() != 12) exit(150);

   message->SetCompressionSettings(-1);
   message->SetCompressionAlgorithm(-1);

   if (message->GetCompressionSettings() != expectedcomplevel) exit(151);

   message->SetCompressionSettings(202);
   message->SetCompressionAlgorithm(0);
   if (message->GetCompressionSettings() != 2) exit(152);

   message->SetCompressionSettings(-1);
   message->SetCompressionAlgorithm(3);
   if (message->GetCompressionSettings() != (300 + expectedcomplevel)) exit(153);

   message->SetCompressionSettings(202);
   message->SetCompressionAlgorithm(99);
   if (message->GetCompressionSettings() != 2) exit(154);

   message->SetCompressionSettings(202);
   message->SetCompressionAlgorithm(1);
   if (message->GetCompressionSettings() != 102) exit(155);

   message->SetCompressionSettings(-1);
   message->SetCompressionLevel(-1);
   if (message->GetCompressionSettings() != 0) exit(156);

   message->SetCompressionSettings(9902);
   message->SetCompressionLevel(0);
   if (message->GetCompressionSettings() != 0) exit(157);

   message->SetCompressionSettings(-1);
   message->SetCompressionLevel(99);
   if (message->GetCompressionSettings() != 99) exit(323);

   message->SetCompressionSettings(302);
   message->SetCompressionLevel(100);
   if (message->GetCompressionSettings() != 399) exit(158);

   message->SetCompressionSettings(1);
   message->SetCompressionLevel(3);
   if (message->GetCompressionSettings() != 3) exit(159);

   message->SetCompressionSettings(201);
   message->SetCompressionLevel(3);
   if (message->GetCompressionSettings() != 203) exit(160);

   TSocket * socket = new TSocket(0, 0, 0);
   testValue = -2;
   socket->SetCompressionSettings(testValue);
   if (socket->GetCompressionSettings() != -1) exit(201);
   if (socket->GetCompressionAlgorithm() != -1) exit(202);
   if (socket->GetCompressionLevel() != -1) exit(203);

   testValue = -1;
   socket->SetCompressionSettings(testValue);
   if (socket->GetCompressionSettings() != -1) exit(204);
   if (socket->GetCompressionAlgorithm() != -1) exit(205);
   if (socket->GetCompressionLevel() != -1) exit(206);

   testValue = 0;
   socket->SetCompressionSettings(testValue);
   if (socket->GetCompressionSettings() != 0) exit(207);
   if (socket->GetCompressionAlgorithm() != 0) exit(208);
   if (socket->GetCompressionLevel() != 0) exit(209);

   testValue = 1112;
   socket->SetCompressionSettings(testValue);
   if (socket->GetCompressionSettings() != 1112) exit(210);
   if (socket->GetCompressionAlgorithm() != 11) exit(211);
   if (socket->GetCompressionLevel() != 12) exit(212);

   socket->SetCompressionSettings(-1);
   socket->SetCompressionAlgorithm(-1);
   if (socket->GetCompressionSettings() != expectedcomplevel) exit(216);

   socket->SetCompressionSettings(202);
   socket->SetCompressionAlgorithm(0);
   if (socket->GetCompressionSettings() != 2) exit(217);

   socket->SetCompressionSettings(-1);
   socket->SetCompressionAlgorithm(3);
   if (socket->GetCompressionSettings() != (300 + expectedcomplevel)) exit(218);

   socket->SetCompressionSettings(202);
   socket->SetCompressionAlgorithm(99);
   if (socket->GetCompressionSettings() != 2) exit(219);

   socket->SetCompressionSettings(202);
   socket->SetCompressionAlgorithm(1);
   if (socket->GetCompressionSettings() != 102) exit(220);

   socket->SetCompressionSettings(-1);
   socket->SetCompressionLevel(-1);
   if (socket->GetCompressionSettings() != 0) exit(221);

   socket->SetCompressionSettings(9902);
   socket->SetCompressionLevel(0);
   if (socket->GetCompressionSettings() != 0) exit(222);

   socket->SetCompressionSettings(-1);
   socket->SetCompressionLevel(99);
   if (socket->GetCompressionSettings() != 99) exit(223);

   socket->SetCompressionSettings(302);
   socket->SetCompressionLevel(100);
   if (socket->GetCompressionSettings() != 399) exit(224);

   socket->SetCompressionSettings(1);
   socket->SetCompressionLevel(3);
   if (socket->GetCompressionSettings() != 3) exit(225);

   socket->SetCompressionSettings(201);
   socket->SetCompressionLevel(3);
   if (socket->GetCompressionSettings() != 203) exit(226);


   Int_t nevent = 400;     // by default create 400 events
   Int_t comp   = 1;       // by default file is compressed
   Int_t split  = 1;       // by default, split Event in sub branches
   Int_t write  = 1;       // by default the tree is filled
   Int_t hfill  = 0;       // by default histograms are not filled
   Int_t read   = 0;
   Int_t arg4   = 1;
   Int_t arg5   = 600;     //default number of tracks per event

   if (argc > 1)  nevent = atoi(argv[1]);
   if (argc > 2)  comp   = atoi(argv[2]);
   if (argc > 3)  split  = atoi(argv[3]);
   if (argc > 4)  arg4   = atoi(argv[4]);
   if (argc > 5)  arg5   = atoi(argv[5]);
   if (arg4 ==  0) { write = 0; hfill = 0; read = 1;}
   if (arg4 ==  1) { write = 1; hfill = 0;}
   if (arg4 ==  2) { write = 0; hfill = 0;}
   if (arg4 == 10) { write = 0; hfill = 1;}
   if (arg4 == 11) { write = 1; hfill = 1;}
   if (arg4 == 20) { write = 0; read  = 1;}  //read sequential
   if (arg4 == 25) { write = 0; read  = 2;}  //read random

   std::stringstream filename;
   filename << "Event" << comp << ".root";

   TFile *hfile;
   TTree *tree;
   Event *event = 0;

   // Fill event, header and tracks with some random numbers
   //   Create a timer object to benchmark this loop
   TStopwatch timer;
   timer.Start();
   Int_t nb = 0;
   Int_t ev;
   Int_t bufsize;
   Double_t told = 0;
   Double_t tnew = 0;
   Int_t printev = 100;
   if (arg5 < 100) printev = 1000;
   if (arg5 < 10)  printev = 10000;

   Track::Class()->IgnoreTObjectStreamer();

//         Read case
   if (read) {
      hfile = new TFile(filename.str().c_str());
      tree = (TTree*)hfile->Get("T");
      TBranch *branch = tree->GetBranch("event");
      branch->SetAddress(&event);
      Int_t nentries = (Int_t)tree->GetEntries();
      // Make sure the number of entries is the expected number
      if (nentries != nevent) {
	std::cerr << "Number of events does not match expected number of events\n";
        exit(29);
      }
      int expectedSize = -1;
      if (comp == 0) expectedSize = 5538619;
#ifdef R__HAS_CLOUDFLARE_ZLIB
      else if (comp == 101) expectedSize = 1239527;
#else
      else if (comp == 101) expectedSize = 1254957;
#endif
      else if (comp == 208) expectedSize = 1088187;
      else if (comp == 301) expectedSize = 1265145;
      else if (comp == 404) expectedSize = 1289623;
      else if (comp == 505) expectedSize = 1156626; // libzstd-1.5.5-1.fc36.x86_64 sees 1162245
#ifdef R__HAS_DEFAULT_LZ4
      else if (comp == 6) expectedSize = 1285037;
#else
      else if (comp == 6) expectedSize = 1208871;
#endif

      if (expectedSize > 0 &&
          (tree->GetZipBytes() > expectedSize + 6000 ||
           tree->GetZipBytes() < expectedSize - 6000)) {
         std::cerr << "Compressed TTree size differs from"
                      " size expected for the input parameters.\n"
                      "The expected size may need tuning as compression "
                      "libraries and other things change.\n";
         std::cerr << "compression setting = " << comp
                   << "  expected compressed TTree size = " << expectedSize
                   << "  actual size = " << tree->GetZipBytes() << std::endl;
         exit(27);
      }
      nevent = TMath::Max(nevent,nentries);
      if (read == 1) {  //read sequential
         for (ev = 0; ev < nevent; ev++) {
            if (ev%printev == 0) {
               tnew = timer.RealTime();
               printf("event:%d, rtime=%f s\n",ev,tnew-told);
               told=tnew;
               timer.Continue();
            }
            //printf("vector size:%d \n",event->GetUshort()->size());
            nb += tree->GetEntry(ev);        //read complete event in memory
            // make sure we read out values in the range we put in when writing
            if (event->GetTemperature() < 20.0 || event->GetTemperature() > 21.0) {
              std::cerr << "Data read from TTree does not match input data\n";
              exit(30);
            }
         }
      } else {    //read random
         Int_t evrandom;
         for (ev = 0; ev < nevent; ev++) {
            if (ev%printev == 0) cout<<"event="<<ev<<endl;
            evrandom = Int_t(nevent*gRandom->Rndm(1));
            nb += tree->GetEntry(evrandom);  //read complete event in memory
         }
      }
   } else {
//         Write case
      // Create a new ROOT binary machine independent file.
      // Note that this file may contain any kind of ROOT objects, histograms,
      // pictures, graphics objects, detector geometries, tracks, events, etc..
      // This file is now becoming the current directory.
      hfile = new TFile(filename.str().c_str(),"RECREATE","TTree benchmark ROOT file");

      // Test get and set functions in TFile
      int testValue = -2;
      hfile->SetCompressionSettings(testValue);
      if (hfile->GetCompressionSettings() != -1) exit(1);
      if (hfile->GetCompressionAlgorithm() != -1) exit(2);
      if (hfile->GetCompressionLevel() != -1) exit(3);

      testValue = -1;
      hfile->SetCompressionSettings(testValue);
      if (hfile->GetCompressionSettings() != -1) exit(4);
      if (hfile->GetCompressionAlgorithm() != -1) exit(5);
      if (hfile->GetCompressionLevel() != -1) exit(6);

      testValue = 0;
      hfile->SetCompressionSettings(testValue);
      if (hfile->GetCompressionSettings() != 0) exit(7);
      if (hfile->GetCompressionAlgorithm() != 0) exit(8);
      if (hfile->GetCompressionLevel() != 0) exit(9);

      testValue = 1112;
      hfile->SetCompressionSettings(testValue);
      if (hfile->GetCompressionSettings() != 1112) exit(10);
      if (hfile->GetCompressionAlgorithm() != 11) exit(11);
      if (hfile->GetCompressionLevel() != 12) exit(12);

      testValue = 1112;
      hfile->SetCompressionSettings(testValue);
      if (hfile->GetCompressionSettings() != 1112) exit(13);
      if (hfile->GetCompressionAlgorithm() != 11) exit(14);
      if (hfile->GetCompressionLevel() != 12) exit(15);

      hfile->SetCompressionSettings(-1);
      hfile->SetCompressionAlgorithm(-1);
      if (hfile->GetCompressionSettings() != expectedcomplevel) exit(16);

      hfile->SetCompressionSettings(202);
      hfile->SetCompressionAlgorithm(0);
      if (hfile->GetCompressionSettings() != 2) exit(17);

      hfile->SetCompressionSettings(-1);
      hfile->SetCompressionAlgorithm(3);
      if (hfile->GetCompressionSettings() != (300 + expectedcomplevel)) exit(18);

      hfile->SetCompressionSettings(202);
      hfile->SetCompressionAlgorithm(99);
      if (hfile->GetCompressionSettings() != 2) exit(19);

      hfile->SetCompressionSettings(202);
      hfile->SetCompressionAlgorithm(1);
      if (hfile->GetCompressionSettings() != 102) exit(20);

      hfile->SetCompressionSettings(-1);
      hfile->SetCompressionLevel(-1);
      if (hfile->GetCompressionSettings() != 0) exit(21);

      hfile->SetCompressionSettings(9902);
      hfile->SetCompressionLevel(0);
      if (hfile->GetCompressionSettings() != 0) exit(22);

      hfile->SetCompressionSettings(-1);
      hfile->SetCompressionLevel(99);
      if (hfile->GetCompressionSettings() != 99) exit(23);

      hfile->SetCompressionSettings(302);
      hfile->SetCompressionLevel(100);
      if (hfile->GetCompressionSettings() != 399) exit(24);

      hfile->SetCompressionSettings(1);
      hfile->SetCompressionLevel(3);
      if (hfile->GetCompressionSettings() != 3) exit(25);

      hfile->SetCompressionSettings(201);
      hfile->SetCompressionLevel(3);
      if (hfile->GetCompressionSettings() != 203) exit(26);

      if (ROOT::CompressionSettings(ROOT::kUseGlobalSetting, 5) != 5) exit(31);
      if (ROOT::CompressionSettings(ROOT::kZLIB, 0) != 100) exit(32);
      if (ROOT::CompressionSettings(ROOT::kZLIB, -1) != 100) exit(36);
      if (ROOT::CompressionSettings(ROOT::kLZMA, 99) != 299) exit(33);
      if (ROOT::CompressionSettings(ROOT::kLZ4, 0) != 400) exit(37);
      if (ROOT::CompressionSettings(ROOT::kLZ4, -1) != 400) exit(38);
      if (ROOT::CompressionSettings(ROOT::kOldCompressionAlgo, 100) != 399) exit(34);
      if (ROOT::CompressionSettings(ROOT::kUndefinedCompressionAlgorithm, 7) != 7) exit(35);

      // Repeat the same tests for get and set functions in TBranch
      TBranch *testBranch = new TBranch();
      testValue = -2;
      testBranch->SetCompressionSettings(testValue);
      if (testBranch->GetCompressionSettings() != -1) exit(101);
      if (testBranch->GetCompressionAlgorithm() != -1) exit(102);
      if (testBranch->GetCompressionLevel() != -1) exit(103);

      testValue = -1;
      testBranch->SetCompressionSettings(testValue);
      if (testBranch->GetCompressionSettings() != -1) exit(104);
      if (testBranch->GetCompressionAlgorithm() != -1) exit(105);
      if (testBranch->GetCompressionLevel() != -1) exit(106);

      testValue = 0;
      testBranch->SetCompressionSettings(testValue);
      if (testBranch->GetCompressionSettings() != 0) exit(107);
      if (testBranch->GetCompressionAlgorithm() != 0) exit(108);
      if (testBranch->GetCompressionLevel() != 0) exit(109);

      testValue = 1112;
      testBranch->SetCompressionSettings(testValue);
      if (testBranch->GetCompressionSettings() != 1112) exit(110);
      if (testBranch->GetCompressionAlgorithm() != 11) exit(111);
      if (testBranch->GetCompressionLevel() != 12) exit(112);

      testValue = 1112;
      testBranch->SetCompressionSettings(testValue);
      if (testBranch->GetCompressionSettings() != 1112) exit(113);
      if (testBranch->GetCompressionAlgorithm() != 11) exit(114);
      if (testBranch->GetCompressionLevel() != 12) exit(115);

      testBranch->SetCompressionSettings(-1);
      testBranch->SetCompressionAlgorithm(-1);
      if (testBranch->GetCompressionSettings() != expectedcomplevel) exit(116);

      testBranch->SetCompressionSettings(202);
      testBranch->SetCompressionAlgorithm(0);
      if (testBranch->GetCompressionSettings() != 2) exit(117);

      testBranch->SetCompressionSettings(-1);
      testBranch->SetCompressionAlgorithm(3);
      if (testBranch->GetCompressionSettings() != (300 + expectedcomplevel)) exit(118);

      testBranch->SetCompressionSettings(202);
      testBranch->SetCompressionAlgorithm(99);
      if (testBranch->GetCompressionSettings() != 2) exit(119);

      testBranch->SetCompressionSettings(202);
      testBranch->SetCompressionAlgorithm(1);
      if (testBranch->GetCompressionSettings() != 102) exit(120);

      testBranch->SetCompressionSettings(-1);
      testBranch->SetCompressionLevel(-1);
      if (testBranch->GetCompressionSettings() != 0) exit(121);

      testBranch->SetCompressionSettings(9902);
      testBranch->SetCompressionLevel(0);
      if (testBranch->GetCompressionSettings() != 0) exit(122);

      testBranch->SetCompressionSettings(-1);
      testBranch->SetCompressionLevel(99);
      if (testBranch->GetCompressionSettings() != 99) exit(123);

      testBranch->SetCompressionSettings(302);
      testBranch->SetCompressionLevel(100);
      if (testBranch->GetCompressionSettings() != 399) exit(124);

      testBranch->SetCompressionSettings(1);
      testBranch->SetCompressionLevel(3);
      if (testBranch->GetCompressionSettings() != 3) exit(125);

      testBranch->SetCompressionSettings(201);
      testBranch->SetCompressionLevel(3);
      if (testBranch->GetCompressionSettings() != 203) exit(126);


      hfile->SetCompressionSettings(comp);

     // Create histogram to show write_time in function of time
     Float_t curtime = 0.5;
     Int_t ntime = nevent/printev + 1;
     TH1F *htime = new TH1F("htime","Real-Time to write versus time",ntime,0,ntime);
     HistogramManager *hm = 0;
     if (hfill) {
        TDirectory *hdir = new TDirectory("histograms", "all histograms");
        hm = new HistogramManager(hdir);
     }

     // Create a ROOT Tree and one superbranch
      TTree *tree = new TTree("T","An example of a ROOT tree");
      tree->SetAutoSave(1000000000);  // autosave when 1 Gbyte written
      //bufsize = 256000;
      bufsize = 64000;
      if (split)  bufsize /= 4;
      event = new Event();
      TBranch *branch = tree->Bronch("event", "Event", &event, bufsize,split);
      branch->SetAutoDelete(kFALSE);
      char etype[20];

      for (ev = 0; ev < nevent; ev++) {
         if (ev%printev == 0) {
            tnew = timer.RealTime();
            printf("event:%d, rtime=%f s\n",ev,tnew-told);
            htime->Fill(curtime,tnew-told);
            curtime += 1;
            told=tnew;
            timer.Continue();
         }

         Float_t sigmat, sigmas;
         gRandom->Rannor(sigmat,sigmas);
         Int_t ntrack   = Int_t(arg5 +arg5*sigmat/120.);
         Float_t random = gRandom->Rndm(1);

         sprintf(etype,"type%d",ev%5);
         event->SetType(etype);
         event->SetHeader(ev, 200, 960312, random);
         event->SetNseg(Int_t(10*ntrack+20*sigmas));
         event->SetNvertex(Int_t(1+20*gRandom->Rndm()));
         event->SetFlag(UInt_t(random+0.5));
         event->SetTemperature(random+20.);

         for(UChar_t m = 0; m < 10; m++) {
            event->SetMeasure(m, Int_t(gRandom->Gaus(m,m+1)));
         }
         for(UChar_t i0 = 0; i0 < 4; i0++) {
            for(UChar_t i1 = 0; i1 < 4; i1++) {
               event->SetMatrix(i0,i1,gRandom->Gaus(i0*i1,1));
            }
         }
         event->GetUshort()->push_back(3);
         event->GetUshort()->push_back(5);
         //printf("vector size:%d \n",event->GetUshort()->size());

         //  Create and Fill the Track objects
         for (Int_t t = 0; t < ntrack; t++) event->AddTrack(random);

         if (write) nb += tree->Fill();  //fill the tree

         if (hm) hm->Hfill(event);      //fill histograms

         event->Clear();
      }
      if (write) {
         hfile->Write();
         tree->Print();
      }

      TestTMessage testMessage(kMESS_OBJECT);
      testMessage.Reset();
      testMessage.SetWriteMode();
      int testSize = 30000000;
      std::vector<char> testVector(testSize, 'a');
      testVector[testSize - 1] = 0;
      TObjString testTString(&testVector[0]);
      testMessage.WriteObject(&testTString);
      testMessage.SetLength();

      char *mbuf = testMessage.Buffer();
      Int_t mlen = testMessage.Length();
      testMessage.SetCompressionSettings(comp);
      if (testMessage.GetCompressionLevel() > 0) {
        if (testMessage.Compress() != 0) exit(98);
        mbuf = testMessage.CompBuffer();
        mlen = testMessage.CompLength();
      }
      char *newbuf = new char[mlen];
      memcpy(newbuf, mbuf,mlen);
      TestTMessage testMessage2(newbuf, mlen); // Uncompress is called in here

   } // end of write case
   //  Stop timer and print results
   timer.Stop();
   Float_t mbytes = 0.000001*nb;
   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();


   printf("\n%d events and %d bytes processed.\n",nevent,nb);
   printf("RealTime=%f seconds, CpuTime=%f seconds\n",rtime,ctime);
   if (read) {
      printf("You read %f Mbytes/Realtime seconds\n",mbytes/rtime);
      printf("You read %f Mbytes/Cputime seconds\n",mbytes/ctime);
   } else {
      printf("compression settings=%d, split=%d, arg4=%d\n",comp,split,arg4);
      printf("You write %f Mbytes/Realtime seconds\n",mbytes/rtime);
      printf("You write %f Mbytes/Cputime seconds\n",mbytes/ctime);
      //printf("file compression factor = %f\n",hfile.GetCompressionFactor());
   }
   hfile->Close();
   return 0;
}
