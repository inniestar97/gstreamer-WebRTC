import { useRef, useEffect, useState } from 'react';

const pc_config: RTCConfiguration = {
  iceServers: [
    {
      urls: "stun:stun.l.google.com:19302",
    },
  ],
  bundlePolicy: 'max-bundle',
};

const localId: string = "ADRemote";
const SIGNALING_SERVER_URL: string = `ws://127.0.0.1:8000/${localId}`;

function App() {

  const [remoteId, setRemoteId] = useState<string>('andromeda');
  const [peerConnectionMap, setPeerConnectionMap] = useState<any>({}); // dictionary
  const [dataChannelMap, setDataChannelMap] = useState<any>({}); // dictionary

  const wsRef = useRef<WebSocket>();
  const videoRef = useRef<HTMLVideoElement>(null);
  const buttonRef = useRef<HTMLButtonElement>(null);
  // const pcRef = useRef<RTCPeerConnection>();
  // const dcRef = useRef<RTCDataChannel>();

  useEffect(() => {

    buttonRef.current!.disabled = true;
    console.log('Connecting to signaling ...');

    // websocket setting
    openSignaling(SIGNALING_SERVER_URL)
      .then((ws) => {
         console.log('WebSocket connected, siganling ready');
         buttonRef.current!.disabled = false;
      })
      .catch((err) => console.error(err));

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    }

  }, []);

  const openSignaling = (url: string) => {
    return new Promise((resolve, reject) => {
      wsRef.current = new WebSocket(url);
      wsRef.current.onopen = () => resolve(wsRef.current);
      wsRef.current.onerror = () => reject(new Error('WebSocket error'));
      wsRef.current.onclose = () => console.error('WebSocket disconnected');

      wsRef.current.onmessage = (msgEvent: MessageEvent) => {
        if (typeof msgEvent.data != 'string') return;

        const message = JSON.parse(msgEvent.data);
        console.log(message);
        const { id, type } = message;

        let pc = peerConnectionMap[id] as RTCPeerConnection;
        if (!pc) {
          console.log(`There's no ${id} before.`);

          if (type !== 'offer')
            return;

          console.log(`Answering to ${id}`);
          pc = createPeerConnection(wsRef.current!, id);
        }
          
        switch (type) { 
        case 'offer': // Maybe, we are not gonna get 'offer' description
        case 'answer':
          pc.setRemoteDescription({
            sdp : message.sdp,
            type : message.type 
          });
          // if (type === 'offer') {
          //   // send answer
          //   sendLocalDescription(wsRef.current!, id, pc, 'answer');
          // }
          break;

        case 'candidate':
          pc.addIceCandidate({
            candidate : message.candidate,
            sdpMid : message.mid
          });
          break;

        default: // We are not handle this section
          break;
        }
          
      };

    });
  };

  const createPeerConnection = (ws: WebSocket, id: string) => {
    const pc = new RTCPeerConnection(pc_config);
    pc.oniceconnectionstatechange = () => console.log(`Connection state: ${pc.iceConnectionState}`);
    pc.onicegatheringstatechange = () => console.log(`Gathering state: ${pc.iceGatheringState}`);

    pc.onicecandidate = (evt: RTCPeerConnectionIceEvent) => {
      if (evt.candidate && evt.candidate.candidate) {
        // Send candidate
        sendLocalCandidate(wsRef.current!, id, evt.candidate);
      }
    }

    pc.ondatachannel = (evt: RTCDataChannelEvent) => {
      const dc = evt.channel;
      console.log(`DataChannel from ${id} received with label "${dc.label}"`);
      setupDataChannel(dc, id);

      dc.send(`Hello from ADRemote`);
    };

    pc.ontrack = (evt: RTCTrackEvent) => {
      console.log("on Track is called");
      if (!videoRef.current) {
        console.log(' video Ref is null ');
        return;
      }
      videoRef.current.srcObject = evt.streams[0];
      videoRef.current.play();
    }


    setPeerConnectionMap((peerConnectionMap[id] = pc));

    return pc;
  }

  const buttonOnClick = async () => {

    console.log(`Offering to ${remoteId}`);
    let pc = createPeerConnection(wsRef.current!, remoteId);

    // Create DataChannel
    const label = "TEST peer connection";
    console.log(`Creating DataChannel with label "${label}"`);
    const dc = pc.createDataChannel(label);
    setupDataChannel(dc, remoteId);

    // Send offer
    sendLocalDescription(wsRef.current!, remoteId, pc, 'offer');

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

    setDataChannelMap((dataChannelMap[id] = dc));

    return dc;
  };

  const sendLocalDescription = (ws: WebSocket, id: string, pc: RTCPeerConnection, type: string) => {
    (type === 'offer' ? pc.createOffer({ offerToReceiveVideo: true }) : pc.createAnswer())
    // (type == 'offer' ? pc.createOffer() : pc.createAnswer())
      .then((desc) => pc.setLocalDescription(desc))
      .then(() => {
        const { sdp, type } = pc.localDescription!;
        console.log(pc.localDescription);
        ws.send(JSON.stringify(
          {
            id,
            type,
            sdp : sdp
          }
        ));
      });
  };

  const sendLocalCandidate = (ws: WebSocket, id: string, cand: RTCIceCandidate) => {
    const { candidate, sdpMid } = cand;
    ws.send(JSON.stringify(
      {
        id,
        type : 'candidate',
        candidate,
        mid : sdpMid
      }
    ))

  };

  return (
    <div className="App">
      <header className="App-header">
        <input type='text' value={ remoteId } onChange={ (e) => { setRemoteId(e.target.value); } }/>
        <button ref={ buttonRef } onClick={ buttonOnClick }>Connect!</button>
      </header>
      <video
        ref={ videoRef }
        style={
          {
            width: 720,
            height: 480,
            margin: 5,
            backgroundColor: "black",
          }
        }
        autoPlay
      />
    </div>
  );

}

export default App;