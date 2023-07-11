#include "SUiController.h"
#include <algorithm>

using namespace std;

SUiController* SUiController::instance = nullptr;
SUiController::SUiController()
{
	cout << "SUiController()" << endl;	
	accountManager = AccountManager::getInstance();
	telephonyManager = TelephonyManager::getInstance();
	
	conferenceDb = ConferenceDb::getInstance();
	contactDb = ContactDb::getInstance();

	accountManager->setUiControl(this);
	telephonyManager->setUiControl(this);
}

SUiController* SUiController::getInstance() {
	if (instance == nullptr) {
		instance = new SUiController();
	}
	return instance;
}

void SUiController::releaseInstance()
{
	telephonyManager->releaseInstance();
	accountManager->releaseInstance();
}

void SUiController::setCallbackWnd(CWnd* wnd)
{
	callbackWnds.push_back(wnd);
	callbackWnds.erase(unique(callbackWnds.begin(), callbackWnds.end()), callbackWnds.end());
}

void SUiController::postMessage(WPARAM wPram, LPARAM lParam)
{
	callbackWnds.erase(remove_if(callbackWnds.begin(), callbackWnds.end(), [](CWnd* p) {return !IsWindow(p->m_hWnd);}), callbackWnds.end());
	for (int i = 0; i < callbackWnds.size(); i++) {
		if (IsWindow(callbackWnds[i]->m_hWnd)) {
			callbackWnds[i]->PostMessageW(UWM_SUI_CONTROLLER, wPram, lParam);
			cout << "SUiController::postMessage() = " << lParam << endl;
		}
	}
}

void SUiController::notify(int type, int result)
{
	postMessage(type, (LPARAM)result);
}

list<ContactData> SUiController::get_AllContacts()
{
	Json::Value data = contactDb->get();
	std::list<ContactData> contactDataList;
	for (const auto& item : data) {
		ContactData contact;
		contact.cid = item["cid"].asString();
		contact.email = item["email"].asString();
		contact.name = item["name"].asString();
		contactDataList.push_back(contact);
	}
	return contactDataList;
}

list<ConferenceData> SUiController::get_AllConferences()
{
	Json::Value data = conferenceDb->get();
	std::list<ConferenceData> conferenceDataList;
	for (const auto& item : data) {
		ConferenceData conference;
		conference.rid = item["rid"].asString();
		conference.dataAndTime = item["dateAndTime"].asUInt64();
		conference.duration = item["duration"].asUInt64();
		for (int i = 0; i < item["participants"].size(); i++) {
			conference.participants.push_back(item["participants"][i].asString());
		}
		conferenceDataList.push_back(conference);
	}
	return conferenceDataList;
}

void SUiController::req_enableContact(CWnd *wnd, string cid, bool enable)
{
	setCallbackWnd(wnd);
	contactDb->setEnable(cid, enable);
	postMessage(MSG_RESPONSE_UPDATE_DATA, 0);
}

void SUiController::req_deleteContact(CWnd* wnd, string cid)
{
	setCallbackWnd(wnd);
	contactDb->remove(cid);
	postMessage(MSG_RESPONSE_UPDATE_DATA, 0);
}

