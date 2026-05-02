(() => {
'use strict';

// ── State ──────────────────────────────────────────────────────────────
let _lineupData  = [];   // all lineups for the current map
let _activeType  = '';   // '' | 'smoke' | 'molotov' | 'flash'
let _debounceId  = null;
let _currentMapId   = '';
let _currentMapLabel = '';
let _maps        = [];   // [{id, label, thumbnail}]
let _adminTags   = [];
let _editId      = '';   // non-empty when editing an existing lineup

// ── DOM refs ───────────────────────────────────────────────────────────
const screenMap    = document.getElementById('screen-map');
const screenSearch = document.getElementById('screen-search');
const mapGrid      = document.getElementById('map-grid');
const mapLabel     = document.getElementById('map-label');
const searchInput  = document.getElementById('search-input');
const resultCount  = document.getElementById('result-count');
const resultsEl    = document.getElementById('results-container');
const btnBack      = document.getElementById('btn-back');
const modal        = document.getElementById('modal');
const modalTitle   = document.getElementById('modal-title');
const modalPos     = document.getElementById('modal-pos');
const modalCross   = document.getElementById('modal-cross');
const modalMeta    = document.getElementById('modal-meta');
const modalVideo   = document.getElementById('modal-video');
const modalClose   = document.getElementById('modal-close');
const modalBack    = document.getElementById('modal-backdrop');
const modalEdit    = document.getElementById('modal-edit');
const modalDelete  = document.getElementById('modal-delete');
const modalAdmin       = document.getElementById('modal-admin');
const adminForm        = document.getElementById('admin-form');
const adminStatus      = document.getElementById('admin-status');
const adminSubmit      = document.getElementById('admin-submit');
const adminTagInput    = document.getElementById('admin-tag-input');
const adminTagsPills   = document.getElementById('admin-tags-pills');

// ── Normalise / tokenise ───────────────────────────────────────────────
function tokenise(raw) {
  return raw.toLowerCase()
    .split(/[\s\-,]+/)
    .map(t => t.trim())
    .filter(t => t.length > 1);
}

// ── OR-relevance search (client-side) ─────────────────────────────────
function search(raw, typeFilter) {
  const tokens = tokenise(raw);
  const pool = typeFilter
    ? _lineupData.filter(l => l.type === typeFilter)
    : _lineupData;

  if (!tokens.length) return pool.slice(0, 50);

  const scored = pool.map(l => {
    const tagSet = new Set(
      l.tags.flatMap(t => { const n = t.toLowerCase(); return [n, ...n.split('-')]; })
    );
    const tagHits   = tokens.reduce((n, tok) => n + (tagSet.has(tok) ? 1 : 0), 0);
    const titleHits = tokens.reduce((n, tok) =>
      n + (l.title.toLowerCase().includes(tok) ? 1 : 0), 0);
    return { l, score: tagHits * 2 + titleHits };
  }).filter(r => r.score > 0);

  scored.sort((a, b) => b.score - a.score);
  return scored.slice(0, 50).map(r => r.l);
}

// ── Badge HTML ─────────────────────────────────────────────────────────
function badge(type) {
  const cls = { smoke: 'badge-smoke', molotov: 'badge-molotov', flash: 'badge-flash' }[type] || '';
  return `<span class="type-badge ${cls}">${type}</span>`;
}

// ── Render result cards ────────────────────────────────────────────────
function renderResults(lineups) {
  if (!lineups.length) {
    resultsEl.innerHTML = '';
    resultCount.textContent = '0 results';
    return;
  }

  resultCount.textContent = `${lineups.length} result${lineups.length !== 1 ? 's' : ''}`;

  const cards = lineups.map((l, idx) => {
    const tags = l.tags.slice(0, 5).map(t =>
      `<span class="tag-pill">${escHtml(t)}</span>`).join('');
    const meta = [l.stance, l.throw_type].filter(Boolean).join(' · ');
    return `<div class="lineup-card" data-idx="${idx}">
  <div class="card-header">${badge(l.type)}<span class="card-title">${escHtml(l.title)}</span></div>
  <div class="card-images">
    <img src="${escHtml(l.position_img)}"  alt="position"  loading="lazy">
    <img src="${escHtml(l.crosshair_img)}" alt="crosshair" loading="lazy">
  </div>
  <div class="card-footer">${tags}${meta ? `<span class="throw-info">${escHtml(meta)}</span>` : ''}</div>
</div>`;
  });

  resultsEl.innerHTML = cards.join('');

  const snapshot = lineups;
  resultsEl.querySelectorAll('.lineup-card').forEach(card => {
    card.addEventListener('click', () => openModal(snapshot[parseInt(card.dataset.idx, 10)]));
  });
}

// ── Detail modal ───────────────────────────────────────────────────────
let _detailLineup = null;

function openModal(l) {
  _detailLineup = l;
  modalTitle.textContent = l.title;
  modalPos.src    = l.position_img;
  modalCross.src  = l.crosshair_img;

  const parts = [];
  if (l.stance)      parts.push(`<span><strong>Stance:</strong> ${escHtml(l.stance)}</span>`);
  if (l.throw_type)  parts.push(`<span><strong>Throw:</strong> ${escHtml(l.throw_type)}</span>`);
  if (l.description) parts.push(`<span>${escHtml(l.description)}</span>`);
  modalMeta.innerHTML = parts.join('');

  if (l.video_url) {
    modalVideo.href = l.video_url;
    modalVideo.classList.remove('hidden');
  } else {
    modalVideo.classList.add('hidden');
  }

  modal.classList.remove('hidden');
  modalClose.focus();
}

function closeModal() {
  modal.classList.add('hidden');
  searchInput.focus();
}

// ── Map grid ───────────────────────────────────────────────────────────
async function loadMaps() {
  mapGrid.innerHTML = '<div class="spinner">Loading maps…</div>';
  try {
    const res = await fetch('/api/maps');
    _maps = await res.json();

    mapGrid.innerHTML = _maps.map(m =>
      `<div class="map-tile" data-id="${escHtml(m.id)}" data-label="${escHtml(m.label)}">
  <img src="${escHtml(m.thumbnail)}" alt="${escHtml(m.label)}" loading="lazy"
       onerror="this.style.visibility='hidden'">
  <div class="map-tile-label">${escHtml(m.label)}</div>
</div>`
    ).join('');

    mapGrid.querySelectorAll('.map-tile').forEach(tile => {
      tile.addEventListener('click', () => selectMap(tile.dataset.id, tile.dataset.label));
    });
  } catch {
    mapGrid.innerHTML = '<div class="spinner">Failed to load maps. Is the server running?</div>';
  }
}

// ── Select a map ───────────────────────────────────────────────────────
async function selectMap(id, label) {
  _currentMapId    = id;
  _currentMapLabel = label;
  showScreen('search');
  mapLabel.textContent    = label;
  searchInput.value       = '';
  resultsEl.innerHTML     = '<div class="spinner">Loading lineups…</div>';
  resultCount.textContent = '';
  searchInput.focus();

  try {
    const res = await fetch(`/api/lineups/${encodeURIComponent(id)}`);
    if (!res.ok) throw new Error('not found');
    _lineupData = await res.json();
  } catch {
    _lineupData = [];
    resultsEl.innerHTML = '<div class="spinner">Failed to load lineups for this map.</div>';
    return;
  }

  renderResults(_lineupData.slice(0, 50));
}

// Reload lineup data for the current map (called after adding a new lineup).
async function reloadCurrentMap() {
  if (!_currentMapId) return;
  try {
    const res = await fetch(`/api/lineups/${encodeURIComponent(_currentMapId)}`);
    if (!res.ok) return;
    _lineupData = await res.json();
    renderResults(search(searchInput.value, _activeType));
  } catch { /* ignore */ }
}

// ── Screen helpers ─────────────────────────────────────────────────────
function showScreen(name) {
  screenMap.classList.toggle('active', name === 'map');
  screenSearch.classList.toggle('active', name === 'search');
}

// ── Debounced search ───────────────────────────────────────────────────
function onSearchInput() {
  clearTimeout(_debounceId);
  _debounceId = setTimeout(() => renderResults(search(searchInput.value, _activeType)), 80);
}

// ── Type filter ────────────────────────────────────────────────────────
document.getElementById('type-filter').addEventListener('click', e => {
  const btn = e.target.closest('.filter');
  if (!btn) return;
  document.querySelectorAll('.filter').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');
  _activeType = btn.dataset.type;
  renderResults(search(searchInput.value, _activeType));
});

// ── Admin modal ────────────────────────────────────────────────────────
function openAdminModal(prefill = null) {
  _editId = prefill ? prefill.id : '';

  adminForm.reset();
  _adminTags = prefill ? [...prefill.tags] : [];
  renderAdminTags();
  document.getElementById('preview-pos').innerHTML   = prefill && prefill.position_img
    ? `<img src="${escHtml(prefill.position_img)}" alt="preview">`
    : '<span class="upload-hint">Click to choose file</span>';
  document.getElementById('preview-cross').innerHTML = prefill && prefill.crosshair_img
    ? `<img src="${escHtml(prefill.crosshair_img)}" alt="preview">`
    : '<span class="upload-hint">Click to choose file</span>';
  adminStatus.textContent = '';
  adminStatus.className   = '';
  adminSubmit.disabled    = false;
  adminSubmit.textContent = _editId ? 'Save Changes' : 'Save Lineup';
  document.querySelector('.admin-header h2').textContent = _editId ? 'Edit Lineup' : 'Add Lineup';

  // Populate map selector
  const sel = document.getElementById('admin-map');
  sel.innerHTML = _maps.map(m =>
    `<option value="${escHtml(m.id)}">${escHtml(m.label)}</option>`
  ).join('');

  if (prefill) {
    sel.value = prefill.map;
    document.getElementById('admin-type').value    = prefill.type    || 'smoke';
    document.getElementById('admin-title').value   = prefill.title   || '';
    document.getElementById('admin-stance').value  = prefill.stance  || 'standing';
    document.getElementById('admin-throw').value   = prefill.throw_type || 'left-click';
    document.getElementById('admin-desc').value    = prefill.description || '';
    document.getElementById('admin-video').value   = prefill.video_url  || '';
  } else if (_currentMapId) {
    sel.value = _currentMapId;
  }

  modalAdmin.classList.remove('hidden');
  document.getElementById('admin-title').focus();
}

function closeAdminModal() {
  modalAdmin.classList.add('hidden');
}

document.getElementById('btn-add').addEventListener('click', openAdminModal);
document.getElementById('admin-close').addEventListener('click', closeAdminModal);
document.getElementById('admin-cancel').addEventListener('click', closeAdminModal);
document.getElementById('modal-admin-backdrop').addEventListener('click', closeAdminModal);

// ── Tag pill management ────────────────────────────────────────────────
function renderAdminTags() {
  adminTagsPills.innerHTML = _adminTags.map(t =>
    `<span class="admin-tag-pill">${escHtml(t)}<button type="button" data-tag="${escHtml(t)}" title="Remove">&#x2715;</button></span>`
  ).join('');
  adminTagsPills.querySelectorAll('button').forEach(btn => {
    btn.addEventListener('click', () => {
      _adminTags = _adminTags.filter(t => t !== btn.dataset.tag);
      renderAdminTags();
    });
  });
}

function addAdminTag(raw) {
  raw.split(/[,\s]+/).forEach(t => {
    t = t.toLowerCase().replace(/[^a-z0-9-]/g, '').trim();
    if (t && !_adminTags.includes(t)) _adminTags.push(t);
  });
  renderAdminTags();
}

adminTagInput.addEventListener('keydown', e => {
  if (e.key === 'Enter' || e.key === ',') {
    e.preventDefault();
    addAdminTag(adminTagInput.value);
    adminTagInput.value = '';
  }
});
// Also add when leaving the field
adminTagInput.addEventListener('blur', () => {
  if (adminTagInput.value.trim()) {
    addAdminTag(adminTagInput.value);
    adminTagInput.value = '';
  }
});

// ── Photo preview ──────────────────────────────────────────────────────
function setupPhotoPreview(inputId, previewId) {
  document.getElementById(inputId).addEventListener('change', e => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = ev => {
      document.getElementById(previewId).innerHTML =
        `<img src="${ev.target.result}" alt="preview">`;
    };
    reader.readAsDataURL(file);
  });
}
setupPhotoPreview('upload-pos',   'preview-pos');
setupPhotoPreview('upload-cross', 'preview-cross');

// ── Upload helper ──────────────────────────────────────────────────────
async function uploadPhoto(inputId) {
  const input = document.getElementById(inputId);
  if (!input.files || !input.files[0]) return '';
  const form = new FormData();
  form.append('file', input.files[0]);
  const res = await fetch('/api/upload', { method: 'POST', body: form });
  if (!res.ok) throw new Error('photo upload failed');
  return (await res.json()).url;
}

// ── Admin form submit ──────────────────────────────────────────────────
adminForm.addEventListener('submit', async e => {
  e.preventDefault();

  const title = document.getElementById('admin-title').value.trim();
  const map   = document.getElementById('admin-map').value;
  if (!title || !map) {
    adminStatus.textContent = 'Map and title are required.';
    adminStatus.className   = 'err';
    return;
  }

  adminSubmit.disabled    = true;
  adminStatus.textContent = 'Uploading photos…';
  adminStatus.className   = '';

  try {
    const [posUrl, crossUrl] = await Promise.all([
      uploadPhoto('upload-pos'),
      uploadPhoto('upload-cross'),
    ]);

    adminStatus.textContent = 'Saving lineup…';

    const payload = {
      map,
      type:          document.getElementById('admin-type').value,
      title,
      description:   document.getElementById('admin-desc').value.trim(),
      tags:          [..._adminTags],
      stance:        document.getElementById('admin-stance').value,
      throw_type:    document.getElementById('admin-throw').value,
      position_img:  posUrl,
      crosshair_img: crossUrl,
      video_url:     document.getElementById('admin-video').value.trim(),
    };

    const url    = _editId ? `/api/lineups/${encodeURIComponent(_editId)}` : '/api/lineups';
    const method = _editId ? 'PUT' : 'POST';
    if (_editId) payload.id = _editId;

    const res = await fetch(url, {
      method,
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify(payload),
    });

    if (!res.ok) {
      const err = await res.json().catch(() => ({}));
      throw new Error(err.error || 'server error');
    }

    adminStatus.textContent = _editId ? '✓ Changes saved!' : '✓ Lineup saved!';
    adminStatus.className   = 'ok';

    // Refresh current map data if the new lineup belongs to it.
    if (map === _currentMapId) await reloadCurrentMap();

    setTimeout(closeAdminModal, 1100);
  } catch (err) {
    adminStatus.textContent = '✗ ' + err.message;
    adminStatus.className   = 'err';
    adminSubmit.disabled    = false;
  }
});

// ── Keyboard shortcuts ─────────────────────────────────────────────────
document.addEventListener('keydown', e => {
  if (e.key === 'Escape') {
    if (!modalAdmin.classList.contains('hidden')) { closeAdminModal(); return; }
    if (!modal.classList.contains('hidden'))      { closeModal();      return; }
    if (screenSearch.classList.contains('active')) showScreen('map');
  }
  if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
    if (screenSearch.classList.contains('active')) {
      e.preventDefault();
      searchInput.focus();
      searchInput.select();
    }
  }
});

// ── Wire up buttons ────────────────────────────────────────────────────
btnBack.addEventListener('click', () => showScreen('map'));
modalClose.addEventListener('click', closeModal);
modalBack.addEventListener('click', closeModal);
searchInput.addEventListener('input', onSearchInput);
modalEdit.addEventListener('click', () => {
  closeModal();
  openAdminModal(_detailLineup);
});

modalDelete.addEventListener('click', async () => {
  if (!_detailLineup) return;
  if (!confirm(`Delete "${_detailLineup.title}"? This cannot be undone.`)) return;

  try {
    const res = await fetch(`/api/lineups/${encodeURIComponent(_detailLineup.id)}`, {
      method: 'DELETE',
    });
    if (!res.ok) throw new Error('server error');
    closeModal();
    await reloadCurrentMap();
  } catch {
    alert('Delete failed. Is the server running?');
  }
});

// ── Utility: safe HTML escaping ────────────────────────────────────────
function escHtml(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

// ── Boot ───────────────────────────────────────────────────────────────
loadMaps();

})();
