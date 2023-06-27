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

  const [socketConnected, setSocketConnected] = useState(false);
  const [remoteId, setRemoteId] = useState("");

  const ws = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>();
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {
    if (!ws.current) {
      ws.current = new WebSocket(SIGNALING_SERVER_URL);
      console.log(ws);

      ws.current.onopen = () => {
        console.log("connected to " + SIGNALING_SERVER_URL);
        setSocketConnected(true);
      };
      
      ws.current.onmessage = (msgEvent: MessageEvent) => {
      }
    }

  }, []);

  const buttonOnClick = () => {
    if (socketConnected && remoteId !== "") {
      pcRef.current = createPeerConnection(pc_config, ws.current, remoteId);
    }
  }

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