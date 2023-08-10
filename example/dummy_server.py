from http.server import SimpleHTTPRequestHandler, HTTPServer

class MyHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        content = b'Small data payload'
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.send_header('Content-Length', len(content))
        self.end_headers()
        self.wfile.write(content)

def run(server_class=HTTPServer, handler_class=MyHandler):
    server_address = ('192.168.1.1', 8000) # Change IP address as needed
    httpd = server_class(server_address, handler_class)
    print(f'Serving HTTP on {server_address[0]} port {server_address[1]} ...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()