import flask
import msgpack
import threading
import Queue
import flask_socketio
import base64
import time

app = flask.Flask(__name__)
app.config['SECRET_KEY'] = 'test'
socketio = flask_socketio.SocketIO(app, async_mode=None)

scan_q = Queue.Queue() # scans
prev_q = Queue.Queue() # image previews

@app.route("/")
def index():
    return flask.render_template('index.htm')

@app.route("/post", methods=["POST"])
def post():
    d = flask.request.get_data()
    u = msgpack.Unpacker()
    u.feed(d)

    # get the JPEG
    x = base64.b64encode(u.unpack())
    
    # next we have text, format and rps (whcih we ignore)
    text = u.unpack()
    fmt = u.unpack()

    if (text):
        scan_q.put((x, text, fmt))
    else:
        prev_q.put(x)

    #print(" ".join(["%x" % ord(c) for c in d]))
    return flask.Response(status=200)

def scan_thread():
    while True:
        x, text, fmt = scan_q.get()
        socketio.emit('scan', {"img": x, "text": "<{}> {}".format(fmt, text)})

def prev_thread():
    while True:
        x = prev_q.get()
        socketio.emit('prev', {"img": x})

if __name__=="__main__":
    thread1 = threading.Thread(target=scan_thread)
    thread1.daemon = True
    thread1.start()

    thread2 = threading.Thread(target=prev_thread)
    thread2.daemon = True
    thread2.start()
    socketio.run(app, debug = True)
