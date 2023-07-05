import React, { useRef, useEffect, useState } from 'react';

const pc_config: RTCConfiguration = {
  iceServers: [
    {
      urls: "stun:stun.l.google.com:19302",
    },
  ],
  bundlePolicy: 'max-bundle',
};
const SIGNALING_SERVER_URL: string = "ws://127.0.0.1:8000/"

function App() {

  const [remoteId, setRemoteId] = useState("");

  const wsRef = useRef<WebSocket>();
  const pcRef = useRef<RTCPeerConnection>();
  const dcRef = useRef<RTCDataChannel | null>();
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {

    // websocket setting
    wsRef.current = new WebSocket(SIGNALING_SERVER_URL + "adRemote");

    // websocket onopen callback
    wsRef.current.onopen = (ev: Event) => {
      console.log("connected to " + SIGNALING_SERVER_URL + ", signaling ready.");
    };

    // websocket onerror callback
    wsRef.current.onerror = (ev: Event) => {
      console.log("WebSocket error");
    };

    // websocket onclose callback
    wsRef.current.onclose = (ev: CloseEvent) => {
      console.log("Websocket Closed");
    };

    // websocket on message callback
    wsRef.current.onmessage = (msgEvent: MessageEvent) => {
      if (!pcRef.current) { // peerconnection check
        console.log("Not peerconnection");
        return;
      }

      if (typeof msgEvent.data !== "string") { // data type check
        console.log("Websocket onMessage data type is not correct.");
        return;
      } 

      const message = JSON.parse(msgEvent.data);

      let id = "id" in message ? message.id : () => {
        console.log("There's no 'id' in JSON");
        return;
      };
      let type = "type" in message ? message.type : () => {
        console.log("There's no 'type' in JSON");
        return;
      }

      if (type === "answer") {
        // Set remote description
        pcRef.current.setRemoteDescription(
          new RTCSessionDescription(message)
        );
      } else if (type === "candidate") {
        // Add ICE candidate to communicate with others
        pcRef.current.addIceCandidate(
          new RTCIceCandidate(message.candidate)
        );
      }
    };

    return () => { // run before start useEffect
      if (wsRef.current) {
        console.log("Websocket not null")
        wsRef.current.close();
      }
      if (pcRef.current) {
        pcRef.current.close();
      }
    }

  }, []);

  const buttonOnClick = async () => {
    if (!wsRef.current || remoteId === "")  return;

    console.log("Offering to " + remoteId + "...");

    // create peer connection
    pcRef.current = new RTCPeerConnection(pc_config);
    if (!pcRef.current || pcRef.current == undefined) {
      console.log("PeerConnection is not constructed");
      return;
    }
    
    pcRef.current.oniceconnectionstatechange = (ev: Event) => {
      console.log("State: " + pcRef.current?.iceConnectionState);
    };

    pcRef.current.onicegatheringstatechange = (ev: Event) => {
      console.log("Gathering state: " + pcRef.current?.iceGatheringState);
    };

    pcRef.current.onsignalingstatechange = (ev: Event) => {
      console.log("signaling state: " + pcRef.current?.signalingState);
    };

    pcRef.current.ontrack = (evt: RTCTrackEvent) => {
      if (!videoRef.current) return;
      videoRef.current.srcObject = evt.streams[0];
      videoRef.current.play();
    }

    // // DataChannel Event
    pcRef.current.ondatachannel = (evt: RTCDataChannelEvent) => {
    }

    const sdp = await pcRef.current.createOffer(
      {
        offerToReceiveVideo: true
      }
    );
    console.log(sdp);
    await pcRef.current.setLocalDescription(new RTCSessionDescription(sdp));
    if (!wsRef.current) {
      console.log("Websocket is not constructed");
      return;
    }
    wsRef.current.send(JSON.stringify(
      {
        "type": sdp.type,
        "sdp": sdp.sdp,
        "id": remoteId
      }
    ));

  };

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