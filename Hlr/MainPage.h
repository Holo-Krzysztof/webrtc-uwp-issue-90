//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::Hlr::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        winrt::Windows::Foundation::IAsyncAction ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

	private:
		winrt::Windows::Foundation::IAsyncAction MainPage::AddTrackToPeerConnectionAsync(winrt::Org::WebRtc::RTCPeerConnection const& pc);

		winrt::Org::WebRtc::RTCPeerConnection localPeer{ nullptr };
		winrt::Org::WebRtc::RTCPeerConnection remotePeer{ nullptr };
		std::vector<Org::WebRtc::IRTCIceCandidate> localCandidates;
		std::vector<Org::WebRtc::IRTCIceCandidate> remoteCandidates;
		winrt::Org::WebRtc::MediaStreamTrack remoteTrack{ nullptr };
    };
}

namespace winrt::Hlr::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
