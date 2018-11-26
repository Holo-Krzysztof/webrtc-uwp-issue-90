#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Org::WebRtc;
using namespace std::chrono;

namespace winrt::Hlr::implementation
{
	//current loopback flow:
	//localPeer only receives
	//remotePeer sends video

	MainPage::MainPage()
	{
		InitializeComponent();
	}

	IAsyncAction MainPage::AddTrackToPeerConnectionAsync(RTCPeerConnection const& pc)
	{
		auto videoDevices = co_await VideoCapturer::GetDevices();
		co_await resume_background(); //this fixes VideoCapturer::Create deadlock

		auto iter = videoDevices.First();

		if (iter.HasCurrent())
		{
			auto deviceInfo = iter.Current().Info();
			hstring name = deviceInfo.Name();
			hstring id = deviceInfo.Id();

			auto capturer = VideoCapturer::Create(name, id);
			auto videoTrackSource = VideoTrackSource::Create(capturer);
			auto track = MediaStreamTrack::CreateVideoTrack(L"REMOTE_VIDEO_TRACK", videoTrackSource);
			remoteTrack = MediaStreamTrack::Cast(track);
			remoteTrack.OnFrameRateChanged([](float frameRate)
			{
				auto msg = std::wstring(L"framerate changed to ") + std::to_wstring(frameRate) + std::wstring(L"\n");
				OutputDebugStringW(msg.c_str());
			});
			pc.AddTrack(track);

			//transceiver doesn't work for some reason...

			//RTCRtpTransceiverInit transceiverInit;
			//transceiverInit.Direction(RTCRtpTransceiverDirection::Sendonly);
			//auto transceiver = remotePeer.AddTransceiver(track, transceiverInit); //there's more overloads, try them.
		}
	}

	IAsyncAction MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
	{
		myButton().Content(box_value(L"Clicked"));

		auto dispatcher = this->Dispatcher();

		auto eventQueue = EventQueueMaker::Bind(dispatcher);
		WebRtcLib::Setup(eventQueue, false, true);

		RTCConfiguration config;
		config.BundlePolicy(RTCBundlePolicy::Balanced);
		config.IceTransportPolicy(RTCIceTransportPolicy::All);
		config.TcpCandidatePolicy(RTCTcpCandidatePolicy::Disabled);
		config.SdpSemantics(RTCSdpSemantics::UnifiedPlan);

		DWORD mainTid = GetCurrentThreadId();
		auto msg = std::wstring(L"main thread id: ") + std::to_wstring(mainTid) + std::wstring(L"\n");
		OutputDebugStringW(msg.c_str());

		localPeer = RTCPeerConnection(config);
		localPeer.OnIceCandidate([&](IRTCPeerConnectionIceEvent const& ice)
		{
			//this needs to be awaited somehow
			remotePeer.AddIceCandidate(ice.Candidate());

			//DWORD tid = GetCurrentThreadId();
			//auto msg = std::wstring(L"localPeer OnIceCandidate from TID ") + std::to_wstring(tid) + std::wstring(L"\n");
			//OutputDebugStringW(msg.c_str());
		});
		localPeer.OnTrack([&](IRTCTrackEvent const& arg)
		{
			OutputDebugStringW(L"localPeer OnTrack\n");
			IMediaStreamTrack track = arg.Track();

			if (track.Kind() == L"video")
			{
				track.Element(nullptr);
				track.Element(MediaElementMaker::Bind(remoteVideoMediaElement()));
			}
		});
		localPeer.OnRemoveTrack([&](IRTCTrackEvent const&)
		{
			OutputDebugStringW(L"localPeer OnRemoveTrack\n");
		});
		localPeer.OnIceGatheringStateChange([&]()
		{
			auto msg = std::wstring(L"localPeer IceGatheringState: ") + std::to_wstring((int32_t)localPeer.IceGatheringState()) + std::wstring(L"\n");
			OutputDebugStringW(msg.c_str());
		});
		//localPeer.OnConnectionStateChange([&]()
		//{
		//	//this is never called
		//	int32_t state = localPeer.ConnectionState_NotAvailable().GetInt32();
		//	auto msg = std::wstring(L"localPeer connection state: ") + std::to_wstring(state) + std::wstring(L"\n");
		//	OutputDebugStringW(msg.c_str());
		//});

		remotePeer = RTCPeerConnection(config);
		remotePeer.OnIceCandidate([&](IRTCPeerConnectionIceEvent const& ice)
		{
			//this too
			//DWORD tid = GetCurrentThreadId();
			//auto msg = std::wstring(L"remotePeer OnIceCandidate from TID ") + std::to_wstring(tid) + std::wstring(L"\n");
			//OutputDebugStringW(msg.c_str());
			
			localPeer.AddIceCandidate(ice.Candidate());
		});
		remotePeer.OnIceGatheringStateChange([&]()
		{
			auto msg = std::wstring(L"remotePeer IceGatheringState: ") + std::to_wstring((int32_t)remotePeer.IceGatheringState()) + std::wstring(L"\n");
			OutputDebugStringW(msg.c_str());
		});
		//remotePeer.OnConnectionStateChange([&]()
		//{
		//	int32_t state = remotePeer.ConnectionState_NotAvailable().GetInt32();
		//	auto msg = std::wstring(L"remotePeer connection state: ") + std::to_wstring(state) + std::wstring(L"\n");
		//	OutputDebugStringW(msg.c_str());
		//});

		//maybe I should set the descriptions when all ICE candidates have been exchanged...or not? there's trickle ICE
		co_await AddTrackToPeerConnectionAsync(remotePeer);

		MediaConstraints constraints;

		auto sdpOffer = co_await remotePeer.CreateOffer(constraints);
		co_await remotePeer.SetLocalDescription(sdpOffer);
		co_await localPeer.SetRemoteDescription(sdpOffer);

		//if we don't wait the video is fucking slow.
		co_await 3s;

		auto sdpAnswer = co_await localPeer.CreateAnswer(constraints);
		co_await localPeer.SetLocalDescription(sdpAnswer);
		co_await remotePeer.SetRemoteDescription(sdpAnswer);
	}
}
