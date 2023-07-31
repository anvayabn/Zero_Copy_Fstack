from http.server import BaseHTTPRequestHandler, HTTPServer

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        print("Received a GET request")
        message = b'Hello, from the Python HTTP server!'
        self.send_response(200)
        self.send_header('Content-Length', len(message))  # Add Content-Length header
        self.end_headers()
        self.wfile.write(message)

    def log_message(self, format, *args):
        print("Received a request: ", format % args)

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, server_address=('10.10.1.2', 80)):
    httpd = server_class(server_address, handler_class)
    print(f"Starting HTTP server on {server_address[0]}:{server_address[1]}")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
