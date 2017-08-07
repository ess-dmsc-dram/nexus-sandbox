#include <stdio.h>
#include <chrono>
#include <vector>

#include <boost/mpi.hpp>
#include <H5Cpp.h>

template <class T>
std::vector<T> read(const H5::H5File &file, const std::string &dataSetName) {
  H5::DataSet dataset = file.openDataSet(dataSetName);
  H5::DataType dataType = dataset.getDataType();
  H5::DataSpace dataSpace = dataset.getSpace();
  std::vector<T> result;
  result.resize(dataSpace.getSelectNpoints());
  dataset.read(result.data(), dataType, dataSpace);
  return result;
}

int main() {
  boost::mpi::environment env;
  boost::mpi::communicator comm;

  H5::H5File file(
      "/home/simon/mantid/nexus/load-performance/sample-files/gzip-level6.nxs",
      H5F_ACC_RDONLY);

  std::vector<int> banks = {22,  23,  24,  42,  43,  44,  62,  63,
                            64,  82,  83,  84,  102, 103, 104, 105,
                            106, 123, 124, 143, 144, 164, 184};

  std::string prefix("entry/bank");
  std::string suffix("_events/");
  std::string name = prefix + std::to_string(banks[comm.rank()]) + suffix;

  const auto start = std::chrono::system_clock::now();

  comm.barrier();
  auto id = read<int32_t>(file, name + "event_id");
  auto tof = read<float>(file, name + "event_time_offset");
  comm.barrier();

  const auto end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed = end - start;
  double seconds = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(
                       elapsed).count() /
                   (1000000000);

  const auto size = tof.size() * sizeof(int32_t) + tof.size() * sizeof(float);
  size_t sum;
  boost::mpi::reduce(comm, size, sum, std::plus<size_t>(), 0);
  if (comm.rank() == 0)
    printf("%f bandwidth %f MB/s size %lu\n", seconds,
           static_cast<double>(sum) / seconds / (1024 * 1024), sum);
}
