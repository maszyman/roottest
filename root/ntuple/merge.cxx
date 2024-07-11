#include "test_common.hxx"

#include <ROOT/RLogger.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RPageStorageFile.hxx>
#include <Compression.h>

#include <TSystem.h>

#include <string>
#include <utility>

using namespace ROOT::Experimental;

static const std::string &HaddPath() {
   const static std::string path = std::string(gSystem->Getenv("ROOTSYS")) + "/bin/hadd";
   return path;
}

template <typename ...Args>
static void HaddExec(Args ...args)
{
   const auto cmdLine = HaddPath() + " " + ((args + " ") + ...);
   gSystem->Exec(cmdLine.c_str());
}

TEST(Merge, MergeWithHadd)
{
   FileRaii kFileName1{"test_rntuple_merge1.root"};
   FileRaii kFileName2{"test_rntuple_merge2.root"};
   FileRaii kFileNameMerged{"test_ntuple_merged.root"};

   auto noPrereleaseWarning =
      RLogScopedVerbosity(ROOT::Experimental::NTupleLog(), ROOT::Experimental::ELogLevel::kError);

   {
      auto model = RNTupleModel::Create();
      model->MakeField<int>("I", 1337);
      model->MakeField<float>("F", 666.f);
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName1.GetPath());
      writer->Fill();
   }
   {
      auto model = RNTupleModel::Create();
      model->MakeField<int>("I", 123);
      model->MakeField<float>("F", 420.f);
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName2.GetPath());
      writer->Fill();
   }

   HaddExec(kFileNameMerged.GetPath(), kFileName1.GetPath(), kFileName2.GetPath());

   {
      auto ntuple = RNTupleReader::Open("ntpl", kFileNameMerged.GetPath());
      auto viewI = ntuple->GetView<int>("I");
      auto viewF = ntuple->GetView<float>("F");
      EXPECT_EQ(viewI(0), 1337);
      EXPECT_EQ(viewI(1), 123);
      EXPECT_FLOAT_EQ(viewF(0), 666.f);
      EXPECT_FLOAT_EQ(viewF(1), 420.f);
   }
}

TEST(Merge, ChangeCompressionWithHadd)
{
   FileRaii kFileName1{"test_rntuple_mergeChangeComp1.root"};
   FileRaii kFileName2{"test_rntuple_mergeChangeComp2.root"};
   FileRaii kFileNameMerged{"test_ntuple_mergedChangeComp.root"};

   auto noPrereleaseWarning =
      RLogScopedVerbosity(ROOT::Experimental::NTupleLog(), ROOT::Experimental::ELogLevel::kError);

   {
      auto model = RNTupleModel::Create();
      model->MakeField<int>("I", 1337);
      model->MakeField<float>("F", 666.f);
      auto opts = RNTupleWriteOptions{};
      opts.SetCompression(ROOT::CompressionSettings(ROOT::RCompressionSetting::EAlgorithm::kZSTD, 5));
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName1.GetPath(), opts);
      writer->Fill();
   }
   {
      auto model = RNTupleModel::Create();
      model->MakeField<int>("I", 123);
      model->MakeField<float>("F", 420.f);
      auto opts = RNTupleWriteOptions{};
      opts.SetCompression(ROOT::CompressionSettings(ROOT::RCompressionSetting::EAlgorithm::kZLIB, 1));
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName2.GetPath(), opts);
      writer->Fill();
   }

   // change compression to 404
   const auto newComp = 404;
   HaddExec("-f" + std::to_string(newComp), kFileNameMerged.GetPath(), kFileName1.GetPath(), kFileName2.GetPath());

   Internal::RPageSourceFile source("ntpl", kFileNameMerged.GetPath(), RNTupleReadOptions());
   source.Attach();

   // print out actual compression
   const auto &desc = source.GetSharedDescriptorGuard();
   auto clusterIter = desc->GetClusterIterable();
   for (const auto &cluster : clusterIter) {
      const auto &cDesc = desc->GetClusterDescriptor(cluster.GetId());
      EXPECT_EQ(cDesc.GetColumnRange(0).fCompressionSettings, newComp);
   }
}