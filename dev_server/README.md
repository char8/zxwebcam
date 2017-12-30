# dev_server

for testing zxwebcam's HTTP POST behaviour.

## requirements

The following python packages:

```
flask
flask_socketio
msgpack-python
```

## usage

```
python2 main.py

# defaults to port 5000

../build/zxwebcam --qr http://localhost:5000 /dev/video0
```

