
self.addEventListener("activate", event => {
    // ensure that the page does not need to be reloaded for the service worker to take effect
    clients.claim();
});


self.addEventListener("fetch", event => {
    // applicable URLs will appear similar to: ..../wlfs/0/AMO.jpg
    const url = new URL(event.request.url);

    if( url.origin === location.origin && url.pathname.startsWith("/wlfs/") ) {
        // use a MessageChannel to request the data from the top frame
        const channel = new MessageChannel();

        clients.matchAll().then(clients => {
            clients.forEach(client => {
                if( client.frameType === "top-level" ) {
                    client.postMessage(url.pathname.substring(1), [ channel.port2 ]);
                }
            });
        });

        event.respondWith(new Promise(resolve => {
            channel.port1.onmessage = event => {
                const virtualFile = event.data;

                if( virtualFile === undefined ) {
                    resolve(new Response("Resource not found", { status: 404 }));
                }

                else {
                    resolve(new Response(virtualFile.content, {
                        status: 200,
                        statusText: 'OK',
                        headers: headers = {
                            "Cross-Origin-Embedder-Policy": "require-corp",
                            "Content-type": virtualFile.contentType
                        }
                    }));
                }
            }
        }));
    }
});
