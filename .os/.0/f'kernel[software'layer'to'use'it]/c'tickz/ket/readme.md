# pfaze kernel

Build the browser module from the repository root:

```powershell
.\testing\build-pfaze-wasm.ps1
```

This compiles `pfaze_kernel.c` with Clang's `wasm32` target and writes
`qck.stdio\operate\pfaze.wasm`. The frontend loads it at startup and exposes
`window.qck.pfaze.reset()`, `tick()`, `run()`, and `read()`.

For example, after opening the frontend:

```js
qck.pfaze.reset({
  quantum: [-768, 1024],
  particle: [1280, -512],
  reach: 2560,
  intent: 1 << 5,
  kinetic: 100n,
  internal: 12n
});
qck.pfaze.run(8);
```