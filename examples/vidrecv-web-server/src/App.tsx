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

  const [remoteId, setRemoteId] = useState("andromeda");
  const [peerConnectionMap, setPeerConnectionMap] = useState({});
  const [dataChannelMap, setDataChannelMap] = useState({});

  const wsRef = useRef<WebSocket>();
  const pcRef = useRef<RTCPeerConnection>();
  const dcRef = useRef<RTCDataChannel>();
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {

    // websocket setting
    openSignaling(SIGNALING_SERVER_URL + "ADRemote");

    // // websocket setting
    // wsRef.current = new WebSocket(SIGNALING_SERVER_URL + "ADRemote");

    // // websocket onopen callback
    // wsRef.current.onopen = (ev: Event) => {
    //   console.log("connected to " + SIGNALING_SERVER_URL + ", signaling ready.");
    // };

    // // websocket onerror callback
    // wsRef.current.onerror = (ev: Event) => {
    //   console.log("WebSocket error");
    // };

    // // websocket onclose callback
    // wsRef.current.onclose = (ev: CloseEvent) => {
    //   console.log("Websocket Closed");
    // };

    // // websocket on message callback
    // wsRef.current.onmessage = (msgEvent: MessageEvent) => {
    //   if (!pcRef.current) { // peerconnection check
    //     console.log("Not peerconnection");
    //     return;
    //   }

    //   if (typeof msgEvent.data !== "string") { // data type check
    //     console.log("Websocket onMessage data type is not correct.");
    //     return;
    //   } 

    //   const message = JSON.parse(msgEvent.data);

    //   let id = "id" in message ? message.id : () => {
    //     console.log("There's no 'id' in JSON");
    //     return;
    //   };
    //   let type = "type" in message ? message.type : () => {
    //     console.log("There's no 'type' in JSON");
    //     return;
    //   }

    //   if (type === "answer") {
    //     // Set remote description
    //     pcRef.current.setRemoteDescription(
    //       new RTCSessionDescription(message)
    //     );
    //   } else if (type === "candidate") {
    //     // Add ICE candidate to communicate with others
    //     pcRef.current.addIceCandidate(
    //       new RTCIceCandidate(message.candidate)
    //     );
    //   }
    // };

    // return () => { // run before start useEffect
    //   if (wsRef.current) {
    //     console.log("Websocket not null")
    //     wsRef.current.close();
    //   }
    //   if (pcRef.current) {
    //     pcRef.current.close();
    //   }
    // }

  }, []);

  const openSignaling = async (url: string) => {
    try {
      return await new Promise((resolve, reject) => {
        wsRef.current = new WebSocket(url);
        console.log("WebSocket Created");
        wsRef.current.onopen = () => resolve(wsRef.current);
        wsRef.current.onerror = () => reject(new Error('WebSocket error'));
        wsRef.current.onclose = () => console.error('WebSocket disconnected');

        wsRef.current.onmessage = (msgEvent: MessageEvent) => {
          if (typeof msgEvent.data != 'string') return;

          const message = JSON.parse(msgEvent.data);
          console.log(message);
          const { id, type } = message;

          let pc = createPeerConnection(wsRef.current!, id);
          
          switch (type) { 
            case 'answer':
              pc.setRemoteDescription({
                sdp: message.sdp,
                type: message.type 
              });
              break;

            case 'candidate':
              pc.addIceCandidate({
                candidate: message.candidate,
                sdpMid: message.mid
              });
              break;

            default:
              break;
          }
          
        };

      });
    } catch (err) {
      return console.error(err);
    }
  }

  const createPeerConnection = (ws: WebSocket, id: string) => {
    const pc = new RTCPeerConnection(pc_config);
    pc.oniceconnectionstatechange = () => console.log(`Connection state: ${pc.iceConnectionState}`);
    pc.onicegatheringstatechange = () => console.log(`Gathering state: ${pc.iceGatheringState}`);

    pc.onicecandidate = (evt: RTCPeerConnectionIceEvent) => {
      if (evt.candidate && evt.candidate.candidate) {
        // Send candidate
        const { candidate, sdpMid } = evt.candidate;
        ws.send(JSON.stringify(
          {
            id,
            type: 'candidate',
            candidate,
            mid: sdpMid
          }
        ));
      }
    }

    pc.ondatachannel = (evt: RTCDataChannelEvent) => {
      const dc = evt.channel;
      console.log(`DataChannel from ${id} received with label "${dc.label}"`);
      setupDataChannel(dc, id);

      dc.send(`Hello from ADRemote`);
    };

    return pc;
  }

  const buttonOnClick = async () => {

    console.log(`Offering to ${remoteId}`);
    const pc = createPeerConnection(wsRef.current!, remoteId);

    // Create DataChannel
    const label = "TEST peer connection";
    console.log(`Creating DataChannel with label "${label}"`);
    const dc = pc.createDataChannel(label);
    setupDataChannel(dc, remoteId);

    sendLocalDescription(wsRef.current!, remoteId, pc, 'offer');
    // if (!wsRef.current || remoteId === "")  return;

    // console.log("Offering to " + remoteId + "...");

    // // create peer connection
    // pcRef.current = new RTCPeerConnection(pc_config);
    // if (!pcRef.current || pcRef.current == undefined) {
    //   console.log("PeerConnection is not constructed");
    //   return;
    // }
    
    // pcRef.current.oniceconnectionstatechange = (ev: Event) => {
    //   console.log("State: " + pcRef.current?.iceConnectionState);
    // };

    // pcRef.current.onicegatheringstatechange = (ev: Event) => {
    //   console.log("Gathering state: " + pcRef.current?.iceGatheringState);
    // };

    // pcRef.current.onsignalingstatechange = (ev: Event) => {
    //   console.log("signaling state: " + pcRef.current?.signalingState);
    // };

    // pcRef.current.ontrack = (evt: RTCTrackEvent) => {
    //   if (!videoRef.current) return;
    //   videoRef.current.srcObject = evt.streams[0];
    //   videoRef.current.play();
    // }

    // // // DataChannel Event
    // const label = "Test from ADRemote";
    // console.log(`Creating DataChannel with label ${label}`);
    // dcRef.current = pcRef.current.createDataChannel(label);

    // const sdp = await pcRef.current.createOffer(
    //   {
    //     offerToReceiveVideo: true
    //   }
    // );
    // console.log(sdp);
    // await pcRef.current.setLocalDescription(new RTCSessionDescription(sdp));
    // if (!wsRef.current) {
    //   console.log("Websocket is not constructed");
    //   return;
    // }
    // wsRef.current.send(JSON.stringify(
    //   {
    //     "type": sdp.type,
    //     "sdp": sdp.sdp,
    //     "id": remoteId
    //   }
    // ));

  };

  const setupDataChannel = (dc: RTCDataChannel, id: string) => {
    dc.onopen = () => {
      console.log(`DataChannel from ${id} open.`);
    };

    dc.onclose = () => { console.log(`DataChannel from ${id} closed`) };

    dc.onmessage =  (evt: MessageEvent<any>) => {
      if (typeof evt.data != 'string') 
        return;
      console.log(`Message from ${id} received: ${evt.data}`);
    }

    return dc;
  }

  const sendLocalDescription = (ws: WebSocket, id: string, pc: RTCPeerConnection, type: string) => {
    (type == 'offer' ? pc.createOffer() : pc.createAnswer())
      .then((desc) => pc.setLocalDescription(desc))
      .then(() => {
        const { sdp, type } = pc.localDescription!;
        ws.send(JSON.stringify(
          {
            id,
            type,
            sdp: sdp
          }
        ));
      });
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