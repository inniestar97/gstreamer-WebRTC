
# Installation

## Follow the below to clone and update submodule

```bash
git clone https://github.com/inniestar97/gstreamer-react.git
cd gstreamer-react
git submodule update --init --recursive --depth 1
```

## POSIX-compliant operating system (LINUX and APPLE MacOS)

### debug

``` bash
mkdir build
cd build
cmake ..
make -j2
```

### release

``` bash
cmake -B build -DUSE_GNUTLS=0 -DUSE_NICE=0 -DCMAKE_BUILD_TYPE=Release
cd build
make -j2
```

# Examples

## gstreamerSender test

Using below code to test gstreamerSender

```bash
gst-launch-1.0 udpsrc address=127.0.0.1 port=5000 caps="application/x-rtp" ! queue ! rtph264depay ! video/x-h264,stream-format=byte-stream ! queue ! avdec_h264 ! queue ! autovideosink
```

1. Run buildied object of gstreamerSender.
2. Run upper command.
3. You can see the Video of WebCAM.
