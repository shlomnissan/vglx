import DefaultTheme from 'vitepress/theme'
import { h } from 'vue'
import GitHubStars from './GitHubStars.vue'
import HeroHorizon from './HeroHorizon.vue'
import './custom.css'

export default {
  ...DefaultTheme,
  Layout() {
    return h(DefaultTheme.Layout, null, {
      'nav-bar-content-after': () => h(GitHubStars),
      'home-hero-before': () => h(HeroHorizon),
    })
  },
}
