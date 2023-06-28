import React, { useRef, useEffect, useState } from 'react';

const pc_config: RTCConfiguration = {
  iceServers: [
    {
      urls: "stun:stun.l.google.com:19302",
    },
  ],
  bundlePolicy: 'max-bundle',
};
const SIGNALING_SERVER_URL: string = "ws://127.0.0.1:8000" 

function App() {

  const [remoteId, setRemoteId] = useState("");

  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>();
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {
    if (!wsRef.current) {
      wsRef.current = new WebSocket(SIGNALING_SERVER_URL);
      console.log(wsRef);

      wsRef.current.onopen = () => {
        console.log("connected to " + SIGNALING_SERVER_URL);
      };
      
      wsRef.current.onmessage = (msgEvent: MessageEvent) => {
      }
    }

  }, []);

  const createOffer = async () => {
    if (!(pcRef.current && wsRef.current)) return;
    try {
      const sdp = await pcRef.current.createOffer({
        offerToReceiveVideo: true,
        offerToReceiveAudio: true,
      });
      await pcRef.current.setLocalDescription(new RTCSessionDescription(sdp));
      wsRef.current.send(JSON.stringify({
        "type": "offer",
        "sdp": sdp
      }));
    }
  };

  const buttonOnClick = () => {
    if (!wsRef.current || remoteId === "")  return;

    console.log("Offering to " + remoteId + "...");
    pcRef.current = createPeerConnection(pc_config, wsRef.current, remoteId);

    /*
     * In this case, button 'clicker' is Offerer.
     * So create 'DataChannel' and 'VideoTrack' to initiate the process
     */
    const testLabel = "This is the react side label for test";
    console.log("Create Datachannel and VideoTrack with label '" + testLabel + "'");






  };

  const createPeerConnection = (config: RTCConfiguration, wws: WebSocket | null, id: string): RTCPeerConnection | null => {

    if (!wws) return null;

    const pc: RTCPeerConnection = new RTCPeerConnection(config);

    pc.oniceconnectionstatechange = (ev: Event) => {
      console.log("State: " + pc.iceConnectionState)
    };

    pc.onicegatheringstatechange = (ev: Event) => {
      console.log("Gathering state: " + pc.iceGatheringState)
      if (pc.iceGatheringState === "complete") {
        JSON.stringify({
        })
      }
    };

    pc.ontrack = (evt: RTCTrackEvent) => {
      if (!videoRef.current) return;
      videoRef.current.srcObject = evt.streams[0];
      videoRef.current.play();
    }

    return pc;
  }
  

  return (
    <div className="App">
      <header className="App-header">
        <input type='text' value={ remoteId } onChange={ (e) => { setRemoteId(e.target.value); } } />
        <button onClick={ buttonOnClick }>Connect!</button>
      </header>
      <video ref={ videoRef } muted></video>
    </div>
  );


}

export default App;