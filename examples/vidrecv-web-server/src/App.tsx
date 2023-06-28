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

      wsRef.current.onopen = (ev: Event) => {
        console.log("connected to " + SIGNALING_SERVER_URL + ", signaling ready.");
      };

      wsRef.current.onerror = (ev: Event) => {
        console.log("WebSocket error");
      };

      wsRef.current.onclose = (ev: CloseEvent) => {
        console.log("Websocket Closed");
      };
      
      wsRef.current.onmessage = (msgEvent: MessageEvent) => {
        if (typeof msgEvent.data === "string") {
          console.log("Websocket onMessage data type is not currect.");
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

        // if (type == "answer") {
        //   const sdp = message.description as string;
        //   pcRef.current?.setRemoteDescription())
        // }


      }
    }

  }, []);

  // const createOffer = async () => {
  //   if (!(pcRef.current && wsRef.current)) return;
  //   try {
  //     const sdp = await pcRef.current.createOffer({
  //       offerToReceiveVideo: true,
  //       offerToReceiveAudio: true,
  //     });
  //     await pcRef.current.setLocalDescription(new RTCSessionDescription(sdp));
  //     wsRef.current.send(JSON.stringify({
  //       "type": "offer",
  //       "sdp": sdp
  //     }));
  //   }
  // };

  const buttonOnClick = () => {
    if (!wsRef.current || remoteId === "")  return;

    console.log("Offering to " + remoteId + "...");
    pcRef.current = createPeerConnection(pc_config, wsRef.current, remoteId);
    if (!pcRef.current) {
      console.log("Peerconnection failed");
      return;
    }

    /*
     * In this case, button 'clicker' is Offerer.
     * So create 'DataChannel' and 'VideoTrack' to initiate the process
     */
    const testLabel = "This is the react side label for test";
    console.log("Create Datachannel and VideoTrack with label '" + testLabel + "'");

    const dc = pcRef.current.createDataChannel(testLabel);

    dc.onopen = () => {
      console.log("DataChannel from " + remoteId + " open.");
      dc.send("Hello from web");
    }

    dc.onclose = () => {
      console.log("DataChannel from " + remoteId + " close.");
    }

    dc.onmessage = (data: any) => {
      if (typeof data === "string") {
        console.log("Message from " + remoteId + " : " + data);
      } else { // show in binary
        let binaryData: Uint8Array = new Uint8Array(data);
        console.log("Binary Message from " + remoteId + " : " + binaryData);
      }
    }

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