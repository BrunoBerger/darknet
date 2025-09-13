#include "darknet_internal.hpp"

#if DARKNET_GPU_ROCM

#include <rocm-core/rocm_version.h>
#include <rocm_smi/rocm_smi.h>

#if __has_include(<sys/utsname.h>)
#include <sys/utsname.h>
#endif


namespace
{
	static auto & cfg_and_state = Darknet::CfgAndState::get();
}

int is_wsl() {

#if __has_include(<sys/utsname.h>)
  struct utsname buf;
  if (uname(&buf) != 0)
    return -1;

  std::string_view sysname(buf.sysname);
  std::string_view release(buf.release);

#ifdef DEBUG
  std::cout << "sysname: " << sysname << "   release: " << release << "   machine: " << buf.machine << "\n";
#endif

  if (sysname != "Linux")
    return 0;

#ifdef __cpp_lib_starts_ends_with
  if (release.ends_with("microsoft-standard-WSL2"))
    return 2;
  if (release.ends_with("-Microsoft"))
    return 1;
#else
  if (release.find("microsoft-standard-WSL2") != std::string::npos)
    return 2;
  if (release.find("-Microsoft") != std::string::npos)
    return 1;
#endif

  return 0;
#endif

  return -1;
}

void Darknet::show_rocm_info()
{
	
	TAT(TATPARMS);
	
	*cfg_and_state.output << "AMD ROCm v" << ROCM_BUILD_INFO << std::endl;
	if (is_wsl() > 0) 
	{
		*cfg_and_state.output << "WSL detected! Skipping the rest of rocm info as it relies on rocm-smi, which does not work in WSL." << std::endl;
		return;
	}

	const auto status1 = rsmi_init(0);

	uint32_t number_of_devices = 0;
	const auto status2 = rsmi_num_monitor_devices(&number_of_devices);

	if (status1 != RSMI_STATUS_SUCCESS or status2 != RSMI_STATUS_SUCCESS)
	{
		const char * msg1 = nullptr;
		const char * msg2 = nullptr;
		rsmi_status_string(status1, &msg1);
		rsmi_status_string(status2, &msg2);

		*cfg_and_state.output
			<< "- status #" << status1 << ": " << (msg1 ? msg1 : "unknown") << std::endl
			<< "- status #" << status2 << ": " << (msg2 ? msg2 : "unknown") << std::endl;
	}

	if (number_of_devices == 0)
	{
		*cfg_and_state.output << Darknet::in_colour(Darknet::EColour::kBrightRed, "AMD GPU not detected!") << std::endl;
	}

	for (uint32_t device_idx = 0; device_idx < number_of_devices; device_idx ++)
	{
		char name[100];
		const size_t len = sizeof(name);
		rsmi_dev_name_get(device_idx, name, len);

		uint64_t memory = 0;
		rsmi_dev_memory_total_get(device_idx, rsmi_memory_type_t::RSMI_MEM_TYPE_VIS_VRAM, &memory);

		*cfg_and_state.output
			<< "=> " << device_idx
			<< ": " << Darknet::in_colour(Darknet::EColour::kBrightGreen, name)
			<< ", " << Darknet::in_colour(Darknet::EColour::kYellow, size_to_IEC_string(memory))
			<< std::endl;
	}

	rsmi_shut_down();
}

#endif // DARKNET_GPU_ROCM
