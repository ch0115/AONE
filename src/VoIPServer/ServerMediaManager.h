#pragma once
#include "IServerMediaManager.h"
#include "../media/MediaManager.h"
#include "../media/VideoMediaPipeline.h"
#include "../media/AudioMediaPipeline.h"
#include "session/ISessionMediaCallback.h"

class ServerMediaManager : public MediaManager, public IServerMediaManager, public ISessionMediaCallback {
private:
	static ServerMediaManager* instance;
	vector<PipeMode> video_pipe_mode_list_;
	vector<PipeMode> audio_pipe_mode_list_;
	OperatingInfo* ServerMediaManager::get_operate_info(void);
	ServerMediaManager(int max_pipeline);
	ContactInfo* get_contact_info(Json::Value add_client_info, bool is_remove);
	string server_ip;
	ISessionMediaCallback* sessionCallback_;
public:
	void setSessionCallback(ISessionMediaCallback* callback);
	void updateClientVideoQuality(Json::Value info) override;
	void startCall(Json::Value room_creat_info);
	void endCall(Json::Value room_remove_info) override;
	void addClient(Json::Value add_client_info) override;
	void removeClient(Json::Value remove_client_info) override;
	Json::Value getMediaProperty() override;
	void notifyVideoQualityChanged(std::string rid, int quality) override;
	static ServerMediaManager* getInstance();
	static void releaseInstance();
};

