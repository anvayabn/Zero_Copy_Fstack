from http.server import BaseHTTPRequestHandler, HTTPServer

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'This is a dummy message from the server!\n')

def run():
    server_address = ('', 8000) # Listen on all available interfaces on port 8000
    httpd = HTTPServer(server_address, MyHandler)
    print('Listening for connections on port 8000...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()