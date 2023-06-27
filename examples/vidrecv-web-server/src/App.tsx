import React, { useRef, useEffect, useState } from 'react';

function App() {

  const pc_config = {
    iceServers: [
      {
        urls: "stun:stun.l.google.com:19302",
      },
    ]
  };
  const SIGNALING_SERVER_URL: string = "ws://127.0.0.1:8000" 
  const [socketConnected, setSocketConnected] = useState(false);


  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    if (!ws.current) {
      ws.current = new WebSocket(SIGNALING_SERVER_URL);
      console.log(ws);

      ws.current.onopen = () => {
        console.log("connected to " + SIGNALING_SERVER_URL);
        setSocketConnected(true);
      };
    }

  }, []);

  return (
    <div className="App">
      <header className="App-header">
      </header>
    </div>
  );
}

export default App;