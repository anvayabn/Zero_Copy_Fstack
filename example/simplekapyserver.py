import http.server
import socketserver

class MyHttpRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.send_header("Connection", "keep-alive")
        self.end_headers()
        self.wfile.write(bytes("<html><head><title>Title goes here.</title></head>", "utf-8"))
        self.wfile.write(bytes("<body><p>This is a test.</p>", "utf-8"))
        self.wfile.write(bytes("<p>You accessed path: %s</p>" % self.path, "utf-8"))
        self.wfile.write(bytes("</body></html>", "utf-8"))
        self.wfile.flush()  # flush the buffer

handler_object = MyHttpRequestHandler

PORT = 8000
my_server = socketserver.TCPServer(("192.168.1.1", PORT), handler_object)

# Star the server
print("serving at port", PORT)
my_server.serve_forever()