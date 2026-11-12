# Servidor bore minimo (mismo protocolo que bore server) para pruebas
import asyncio, json, uuid, sys
conns = {}
async def pipe(r, w):
    try:
        while True:
            d = await r.read(65536)
            if not d: break
            w.write(d); await w.drain()
    except Exception: pass
    finally:
        try: w.close()
        except Exception: pass
async def readmsg(r):
    b = await r.readuntil(b'\0'); return json.loads(b[:-1])
async def send(w, o):
    w.write(json.dumps(o).encode() + b'\0'); await w.drain()
async def control(r, w):
    m = await readmsg(r)
    if 'Hello' in m:
        q = asyncio.Queue()
        async def on_in(ir, iw): await q.put((ir, iw))
        srv = await asyncio.start_server(on_in, '127.0.0.1', 0)
        port = srv.sockets[0].getsockname()[1]
        await send(w, {'Hello': port}); print('hello ->', port, flush=True)
        try:
            while True:
                try:
                    ir, iw = await asyncio.wait_for(q.get(), 0.5)
                    i = str(uuid.uuid4()); conns[i] = (ir, iw)
                    await send(w, {'Connection': i}); print('connection', i, flush=True)
                except asyncio.TimeoutError:
                    await send(w, 'Heartbeat')
        except Exception as e: print('control fin', e, flush=True); srv.close()
    elif 'Accept' in m:
        ir, iw = conns.pop(m['Accept'])
        print('accept', m['Accept'], flush=True)
        await asyncio.gather(pipe(r, iw), pipe(ir, w))
async def main():
    s = await asyncio.start_server(control, '127.0.0.1', 7835)
    await s.serve_forever()
asyncio.run(main())
