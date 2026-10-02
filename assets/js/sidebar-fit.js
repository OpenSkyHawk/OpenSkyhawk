// Fit the sticky sidebars to the space actually free on screen.
//
// Material sizes each sidebar's scroll area for a header that stays fixed at the top of the
// window. Ours scrolls away (extra.css pins the sidebars 1rem from the top instead), so near the
// footer Material's height comes out ~header-height too short and cuts the nav off. Measure the
// real space — from the scroll area's top to the end of the page content or the window, whichever
// comes first — and hand it to extra.css as --os-sidebar-height. Material never touches that
// variable, so its own inline height can't override it.
(() => {
  const GAP = 16; // px kept clear above the footer / window edge

  const fit = () => {
    const inner = document.querySelector(".md-main__inner");
    if (!inner) return;
    const bottom = Math.min(window.innerHeight, inner.getBoundingClientRect().bottom) - GAP;
    for (const wrap of document.querySelectorAll(".md-sidebar__scrollwrap")) {
      const height = Math.max(0, bottom - wrap.getBoundingClientRect().top);
      wrap.style.setProperty("--os-sidebar-height", `${height}px`);
    }
  };

  for (const event of ["scroll", "resize", "load"]) {
    window.addEventListener(event, fit, { passive: true });
  }
  fit();
})();
