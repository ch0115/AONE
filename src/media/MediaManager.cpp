#include "MediaManager.h"
//#include <winsock2.h>

#include "common/logger.h"
#include "media_config.h"

namespace media {

VideoPresetLevel MediaManager::ShouldChangeVideoQuality(const VideoPresetType& current_preset,
	const PipelineMonitorable::RtpStats& stats) {
	// do not use current_preset.
	VideoPresetLevel cur_level = vqa_data_per_room_[stats.rid].cur_level; //current_preset.level;
	VideoPresetLevel ret = cur_level;

	if (!vqa_data_per_room_[stats.rid].num_clients)
		return cur_level;

	LOG_OBJ_LOG() << "cur_bitrate " << current_preset.bitrate << ", num_lost " << stats.num_lost
		<< ", num_late " << stats.num_late << ", avg_jitter " << stats.avg_jitter_us << " us" << std::endl;

	vqa_data_per_room_[stats.rid].elapsed_time = clock() - vqa_data_per_room_[stats.rid].begin_time;
	if (vqa_data_per_room_[stats.rid].elapsed_time > CLOCKS_PER_SEC) {
		uint64_t num_lost_per_sec = stats.num_lost - vqa_data_per_room_[stats.rid].num_lost_last;

		LOG_OBJ_INFO() << "[rid:" << stats.rid << "] cur_bitrate " << current_preset.bitrate
			<< ", num_lost " << stats.num_lost << ", num_late " << stats.num_late
			<< ", avg_jitter " << stats.avg_jitter_us << " us"
			<< ", num_lost_per_last_1sec " << num_lost_per_sec << std::endl;

		//if ((num_lost_per_sec/vqa_data_per_room_[stats.rid].num_clients) >
		//	MediaConfig::ThresholdOfNumLostForChangingVideoQuality()) {
		//	LOG_OBJ_WARN() << "[rid:" << stats.rid << "] avg num_lost(num_lost/clients) "
		//		<< num_lost_per_sec / vqa_data_per_room_[stats.rid].num_clients << " is bigger than threshold("
		//		<< MediaConfig::ThresholdOfNumLostForChangingVideoQuality() << "). Need to Notify!" << std::endl;
		if ((num_lost_per_sec > MediaConfig::ThresholdOfNumLostForChangingVideoQuality())) {
			LOG_OBJ_WARN() << "[rid:" << stats.rid << "] num_lost_per_sec(" << num_lost_per_sec
				<< ") is bigger than threshold(" << MediaConfig::ThresholdOfNumLostForChangingVideoQuality()
				<< "). Need to Notify!" << std::endl;


			if (cur_level > VideoPresetLevel::kVideoPresetOff)
				ret = VideoPresetType::Lower(cur_level);
		}

		vqa_data_per_room_[stats.rid].begin_time = clock();
		vqa_data_per_room_[stats.rid].num_lost_last = stats.num_lost;
	}

	return ret;
}

MediaManager::MediaManager(int max_pipeline) {
	max_pipleline_ = max_pipeline;
	// TODO: codec/encryption 관련 등록 기능 추가
}

void MediaManager::SetRtpJitterBufferLatency(unsigned int latency) {
	for (auto room : pipelineMap_) {
		auto av_pipelines = room.second;
		for (auto pipeline : av_pipelines.video_pipelines) {
			pipeline->set_rtp_jitter_buffer_latency(latency);
		}
		for (auto pipeline : av_pipelines.audio_pipelines) {
			pipeline->set_rtp_jitter_buffer_latency(latency);
		}
	}
}

string MediaManager::get_ip_address(void) {
	return "127.0.0.1";
#if 0
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return "";
	}

	char hostName[256];
	if (gethostname(hostName, sizeof(hostName)) == 0) {
		struct hostent* host = gethostbyname(hostName);
		if (host != nullptr) {
			struct in_addr addr;
			memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
			return inet_ntoa(addr);
		}
	}
	return "";
#endif
}

vector<string> MediaManager::split(const string& str, char delim)
{
	auto i = 0;
	vector<string> list;

	auto pos = str.find(delim);
	while (pos != string::npos)
	{
		list.push_back(str.substr(i, pos - i));
		i = ++pos;
		pos = str.find(delim, pos);
	}

	list.push_back(str.substr(i, str.length()));
	return list;
}

int MediaManager::get_port_number(string dest_ip, string type) {
	string ip_part = "000";

	vector<string> nums = split(dest_ip, '.');
	if (nums.size() == 4) {
		ip_part = std::to_string(stoi(nums[2])%10);
		if (nums[3].size() == 1) {
			ip_part += "0" + nums[3];
		} else {
			ip_part += std::to_string(stoi(nums[3]) % 100);
		}
	}

	string type_number;
	if (type == "video") type_number = "1";
	else if(type == "audio") type_number = "2";
	else  type_number = "0";

	string port_num = "1" + ip_part + type_number;
	LOG_OBJ_INFO() << "IP : " << dest_ip << " port : " << port_num << endl;
	return stoi(port_num);
}

bool MediaManager::checkValidRID(string rid) {
	if (pipelineMap_.find(rid) == pipelineMap_.end()) {
		return FALSE;
	}
	return TRUE;
}

vector<VideoMediaPipeline*> MediaManager::getVideoPipeLine(string rid) {
	vector<VideoMediaPipeline*> null_vector;
	if (!checkValidRID(rid)) {
		LOG_OBJ_INFO() << " Invalid RID (" << rid << ")" << endl;
		return null_vector;
	}
	return pipelineMap_[rid].video_pipelines;
}

vector<AudioMediaPipeline*> MediaManager::getAudioPipeLine(string rid) {
	vector<AudioMediaPipeline*> null_vector;
	if (!checkValidRID(rid)) {
		LOG_OBJ_INFO() << " Invalid RID (" << rid << ")" << endl;
		return null_vector;
	}
	return pipelineMap_[rid].audio_pipelines;
}

void MediaManager::end_call_with_rid(string rid)
{
	vector<VideoMediaPipeline*> video_piplelines = getVideoPipeLine(rid);
	for (auto pipeline : video_piplelines) {
		if (pipeline == NULL) continue;
		pipeline->end_call();
	}

	vector<AudioMediaPipeline*> audio_piplelines = getAudioPipeLine(rid);
	for (auto pipeline : audio_piplelines) {
		if (pipeline == NULL) continue;
		pipeline->end_call();
	}
	pipelineMap_.erase(rid);
	vqa_data_per_room_.erase(rid);
}

void MediaManager::OnRtpStats(const VideoPresetType& current_preset, const PipelineMonitorable::RtpStats& stats) {
	LOG_LOG("IN");

	VideoPresetLevel next_preset_level = ShouldChangeVideoQuality(current_preset, stats);
	//if (next_preset_level != current_preset.level) {
	// Do not use the level of current_preset of pipeline because we decided to keep server's video quality due to no time for testing
	if (next_preset_level != vqa_data_per_room_[stats.rid].cur_level) {
		LOG_OBJ_WARN() << "Request to change video quality! current " << static_cast<int>(vqa_data_per_room_[stats.rid].cur_level)
			<< " to " << static_cast<int>(next_preset_level) << std::endl;

		notifyVideoQualityChangeNeeded(stats.rid, next_preset_level);
	}
}

bool MediaManager::OnAudioBuffer(const AudioBuffer& buffer, size_t frames_per_buffer) {
	return true;
}

} // namespace media