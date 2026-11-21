<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'

const canvas = ref<HTMLCanvasElement | null>(null)
const grain = ref<HTMLDivElement | null>(null)

const MIN_VIEW_WIDTH = 0.85
const HORIZON_GAP = 0.09
const HORIZON_BELOW_CENTER = 0.023
const FALLBACK_HORIZON = 0.64

const VERT = `#version 300 es
in vec2 a_pos;
void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }`

const FRAG = `#version 300 es
precision highp float;
uniform vec2 u_center;
uniform float u_zoom;
uniform float u_time;
out vec4 o_color;

const vec3 BG     = vec3(0.106, 0.106, 0.122);
const vec3 BLUE   = vec3(0.149, 0.682, 1.000);
const vec3 VIOLET = vec3(0.545, 0.361, 0.965);
const vec3 PINK   = vec3(0.925, 0.282, 0.600);

const float BRIGHTNESS = 0.64;
const float SPIN = 0.9;
const float SPIN_PERIOD = 6.0;

const float DISK_INNER = 1.9;
const float DISK_OUTER = 13.0;

const float CAM_DISTANCE = 14.0;
const float CAM_ELEVATION = radians(3.2);
const float CAM_FOCAL = 3.23;
const vec3 CAM_FORWARD = vec3(0.0, -sin(CAM_ELEVATION), cos(CAM_ELEVATION));
const vec3 CAM_UP = vec3(0.0, cos(CAM_ELEVATION), sin(CAM_ELEVATION));
const vec3 CAM_POS = -CAM_DISTANCE * CAM_FORWARD;

const int MAX_STEPS = 170;
const float ESCAPE_DISTANCE = 24.0;

float hash(vec3 p) {
  p = fract(p * 0.1031);
  p += dot(p, p.zyx + 31.32);
  return fract((p.x + p.y) * p.z);
}

float valueNoise(vec3 p) {
  vec3 cell = floor(p), t = fract(p);
  t = t * t * (3.0 - 2.0 * t);
  float x00 = mix(hash(cell), hash(cell + vec3(1.0, 0.0, 0.0)), t.x);
  float x10 = mix(hash(cell + vec3(0.0, 1.0, 0.0)), hash(cell + vec3(1.0, 1.0, 0.0)), t.x);
  float x01 = mix(hash(cell + vec3(0.0, 0.0, 1.0)), hash(cell + vec3(1.0, 0.0, 1.0)), t.x);
  float x11 = mix(hash(cell + vec3(0.0, 1.0, 1.0)), hash(cell + vec3(1.0, 1.0, 1.0)), t.x);
  return mix(mix(x00, x10, t.y), mix(x01, x11, t.y), t.z);
}

vec2 rotate(vec2 p, float angle) {
  float c = cos(angle), s = sin(angle);
  return vec2(c * p.x + s * p.y, -s * p.x + c * p.y);
}

float gasDensity(vec2 radial, float radius, float angle, float seed) {
  vec3 offset = vec3(seed * 7.13, seed * 3.71, 0.0);
  float coarse = valueNoise(vec3(rotate(radial, angle) * 1.6, radius * 1.9) + offset);
  float fine = valueNoise(vec3(rotate(radial, angle * 0.55) * 4.0, radius * 4.6 + 9.0) + offset);
  return 0.62 * coarse + 0.38 * fine;
}

vec4 sampleDisk(vec3 hit, vec3 rayDir) {
  float radius = length(hit.xz);
  if (radius < DISK_INNER || radius > DISK_OUTER) return vec4(0.0);
  vec2 radial = hit.xz / radius;

  float angularSpeed = SPIN * pow(DISK_INNER / radius, 1.5);
  float cycle = u_time / SPIN_PERIOD;
  float phaseA = fract(cycle);
  float phaseB = fract(cycle + 0.5);
  float blend = 1.0 - abs(2.0 * phaseA - 1.0);
  float density = blend * gasDensity(radial, radius, angularSpeed * phaseA * SPIN_PERIOD, floor(cycle))
                + (1.0 - blend) * gasDensity(radial, radius, angularSpeed * phaseB * SPIN_PERIOD, floor(cycle + 0.5) + 0.5);
  density = clamp(0.5 + (density - 0.5) * (1.0 + 0.4 * (1.0 - abs(2.0 * blend - 1.0))), 0.0, 1.0);
  float streaks = 0.30 + 1.25 * density * density;

  float falloff = pow(DISK_INNER / radius, 3.0);
  float edges = smoothstep(DISK_INNER, DISK_INNER + 0.35, radius) * (1.0 - smoothstep(DISK_OUTER * 0.5, DISK_OUTER, radius));

  float doppler = dot(vec3(-radial.y, 0.0, radial.x), -rayDir);
  float beaming = pow(1.0 + 0.30 * doppler, 3.0);

  vec3 color = mix(vec3(0.55, 0.80, 1.0), BLUE, smoothstep(DISK_INNER, DISK_INNER * 1.8, radius));
  color = mix(color, VIOLET, smoothstep(DISK_INNER * 1.4, DISK_INNER * 3.0, radius));
  color = mix(color, PINK, smoothstep(DISK_INNER * 2.5, DISK_OUTER * 0.7, radius) * 0.8);
  color = mix(color, PINK, clamp(-doppler, 0.0, 1.0) * 0.45);
  color = mix(color, vec3(0.70, 0.88, 1.0), clamp(doppler, 0.0, 1.0) * 0.12);

  float opacity = edges * mix(0.25, 0.96, pow(DISK_INNER / radius, 0.9));
  return vec4(color * falloff * edges * streaks * beaming * 2.2, opacity);
}

void main() {
  vec2 screen = (gl_FragCoord.xy - u_center) / u_zoom;
  vec3 pos = CAM_POS;
  vec3 velocity = normalize(CAM_FORWARD * CAM_FOCAL + vec3(screen.x, 0.0, 0.0) + CAM_UP * screen.y);

  vec3 angMom = cross(pos, velocity);
  float angMomSq = dot(angMom, angMom);
  vec3 light = vec3(0.0);
  float transmittance = 1.0;
  float haze = 0.0;
  int crossings = 0;

  for (int i = 0; i < MAX_STEPS; i++) {
    float distSq = dot(pos, pos);
    if (distSq < 1.0) break;
    float dist = sqrt(distSq);
    float stepSize = clamp(0.09 * dist, 0.025, 0.8);
    velocity += -1.5 * angMomSq * pos / (distSq * distSq * dist) * stepSize;
    vec3 nextPos = pos + velocity * stepSize;

    if (pos.y * nextPos.y < 0.0 && crossings < 2) {
      crossings++;
      vec3 hit = mix(pos, nextPos, pos.y / (pos.y - nextPos.y));
      vec4 disk = sampleDisk(hit, normalize(velocity));
      light += transmittance * disk.rgb;
      transmittance *= 1.0 - disk.a;
      if (transmittance < 0.01) break;
    }

    float axisDist = length(pos.xz);
    haze += transmittance * stepSize * exp(-abs(pos.y) * 2.2) * pow(DISK_INNER / max(axisDist, DISK_INNER), 2.2);
    pos = nextPos;
    if (dist > ESCAPE_DISTANCE && dot(pos, velocity) > 0.0) break;
  }
  light += mix(BLUE, VIOLET, 0.45) * haze * 0.035;

  float peak = max(max(light.r, light.g), max(light.b, 1e-4));
  light *= (1.0 - exp(-peak)) / peak;
  o_color = vec4(BG + light * BRIGHTNESS * (1.0 - BG), 1.0);
}`

let gl: WebGL2RenderingContext | null = null
let uCenter: WebGLUniformLocation | null = null
let uZoom: WebGLUniformLocation | null = null
let uTime: WebGLUniformLocation | null = null
let frameRequest = 0
let lastFrameTime = 0
let elapsed = 0
let onScreen = true
let visibilityObserver: IntersectionObserver | null = null
let resizeObserver: ResizeObserver | null = null
let reducedMotion: MediaQueryList | null = null

function compile(type: number, source: string) {
  const shader = gl!.createShader(type)!
  gl!.shaderSource(shader, source)
  gl!.compileShader(shader)
  if (!gl!.getShaderParameter(shader, gl!.COMPILE_STATUS)) {
    console.error('[HeroHorizon]', gl!.getShaderInfoLog(shader))
    gl!.deleteShader(shader)
    return null
  }
  return shader
}

function init(el: HTMLCanvasElement) {
  gl = el.getContext('webgl2', { antialias: false, alpha: false, depth: false, powerPreference: 'high-performance' })
  if (!gl) return false
  const vertexShader = compile(gl.VERTEX_SHADER, VERT)
  const fragmentShader = compile(gl.FRAGMENT_SHADER, FRAG)
  if (!vertexShader || !fragmentShader) return false
  const program = gl.createProgram()!
  gl.attachShader(program, vertexShader)
  gl.attachShader(program, fragmentShader)
  gl.linkProgram(program)
  gl.deleteShader(vertexShader)
  gl.deleteShader(fragmentShader)
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    console.error('[HeroHorizon]', gl.getProgramInfoLog(program))
    return false
  }
  gl.useProgram(program)

  gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer())
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW)
  const position = gl.getAttribLocation(program, 'a_pos')
  gl.enableVertexAttribArray(position)
  gl.vertexAttribPointer(position, 2, gl.FLOAT, false, 0, 0)

  uCenter = gl.getUniformLocation(program, 'u_center')
  uZoom = gl.getUniformLocation(program, 'u_zoom')
  uTime = gl.getUniformLocation(program, 'u_time')
  return true
}

function draw() {
  if (!gl) return
  gl.uniform1f(uTime, elapsed)
  gl.drawArrays(gl.TRIANGLES, 0, 3)
}

function resize() {
  const el = canvas.value
  if (!gl || !el) return
  const width = el.clientWidth
  const height = el.clientHeight
  el.width = width
  el.height = height
  gl.viewport(0, 0, width, height)

  const zoom = Math.min(height, width / MIN_VIEW_WIDTH)
  const buttons = document.querySelector('.VPHomeHero .actions')
  const horizon = buttons
    ? buttons.getBoundingClientRect().bottom - el.getBoundingClientRect().top + HORIZON_GAP * zoom
    : FALLBACK_HORIZON * height
  gl.uniform2f(uCenter, width / 2, height - horizon + HORIZON_BELOW_CENTER * zoom)
  gl.uniform1f(uZoom, zoom)
  draw()
}

function animating() {
  return onScreen && !reducedMotion?.matches
}

function frame(now: number) {
  elapsed += Math.min((now - lastFrameTime) / 1000, 0.1)
  lastFrameTime = now
  draw()
  frameRequest = animating() ? requestAnimationFrame(frame) : 0
}

function startIfIdle() {
  if (frameRequest || !animating()) return
  lastFrameTime = performance.now()
  frameRequest = requestAnimationFrame(frame)
}

function makeGrain(el: HTMLDivElement) {
  const size = 200
  const dpr = window.devicePixelRatio || 1
  const tile = document.createElement('canvas')
  tile.width = tile.height = Math.round(size * dpr)
  const context = tile.getContext('2d')
  if (!context) return
  const pixels = context.createImageData(tile.width, tile.height)
  for (let i = 0; i < pixels.data.length; i += 4) {
    pixels.data[i] = pixels.data[i + 1] = pixels.data[i + 2] = 255 * Math.random()
    pixels.data[i + 3] = 255
  }
  context.putImageData(pixels, 0, 0)
  el.style.backgroundImage = `url(${tile.toDataURL()})`
  el.style.backgroundSize = `${size}px ${size}px`
}

onMounted(() => {
  const el = canvas.value
  if (!el || !init(el)) return
  if (grain.value) makeGrain(grain.value)

  reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)')
  reducedMotion.addEventListener('change', startIfIdle)

  visibilityObserver = new IntersectionObserver(([entry]) => {
    onScreen = entry.isIntersecting
    startIfIdle()
  })
  visibilityObserver.observe(el)

  resizeObserver = new ResizeObserver(resize)
  resizeObserver.observe(el)
  const hero = document.querySelector('.VPHomeHero')
  if (hero) resizeObserver.observe(hero)

  resize()
  el.classList.add('ready')
  startIfIdle()
})

onBeforeUnmount(() => {
  cancelAnimationFrame(frameRequest)
  frameRequest = 0
  visibilityObserver?.disconnect()
  resizeObserver?.disconnect()
  reducedMotion?.removeEventListener('change', startIfIdle)
  gl?.getExtension('WEBGL_lose_context')?.loseContext()
  gl = null
})
</script>

<template>
  <div class="hero-horizon" aria-hidden="true">
    <canvas ref="canvas" />
    <div ref="grain" class="grain" />
  </div>
</template>

<style scoped>
.hero-horizon {
  position: absolute;
  top: calc(var(--vp-nav-height) * -1);
  left: 0;
  width: 100%;
  height: 780px;
  overflow: hidden;
  pointer-events: none;
  -webkit-mask-image: linear-gradient(to bottom, #000 0%, #000 72%, transparent 100%);
  mask-image: linear-gradient(to bottom, #000 0%, #000 72%, transparent 100%);
}

canvas {
  display: block;
  width: 100%;
  height: 100%;
  opacity: 0;
  transition: opacity 1.2s ease;
}

canvas.ready {
  opacity: 1;
}

.grain {
  position: absolute;
  inset: 0;
  mix-blend-mode: overlay;
  opacity: 0.077;
}

@media (max-width: 640px) {
  .hero-horizon {
    height: 640px;
  }
}
</style>
