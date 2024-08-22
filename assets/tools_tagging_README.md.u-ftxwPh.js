import{_ as e,c as a,o as s,V as n}from"./chunks/framework.gBlNPWt_.js";const m=JSON.parse('{"title":"Tagging new versions","description":"","frontmatter":{},"headers":[],"relativePath":"tools/tagging/README.md","filePath":"tools/tagging/README.md","lastUpdated":1724329493000}'),t={name:"tools/tagging/README.md"},i=n(`<h1 id="tagging-new-versions" tabindex="-1">Tagging new versions <a class="header-anchor" href="#tagging-new-versions" aria-label="Permalink to &quot;Tagging new versions&quot;">​</a></h1><h2 id="general" tabindex="-1">General <a class="header-anchor" href="#general" aria-label="Permalink to &quot;General&quot;">​</a></h2><p>The file tools/git/aliases.sh contains various git aliases which are useful when using git, and may be used by the merge scripts.</p><p>The file env.sh.example contains some example environment variable settings. Copy that locally to env.sh and modify it to match your local setup.</p><p>Before running any of the other scripts, process the contents of that file as a source file</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>. env.sh</span></span></code></pre></div><p>to initialize the common environment variables.</p><p>The following repositories should be checked out in a directory reserved for merging and tagging (default for scripts is ~/git):</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>git clone https://github.com/hpcc-systems/eclide.git</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/HPCC-JAPIs.git</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/Spark-HPCC.git</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/LN.git ln</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/HPCC-Platform.git hpcc</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/helm-chart.git</span></span></code></pre></div><p>The following are required for builds prior to 8.12.x</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>git clone https://github.com/hpcc-systems/nagios-monitoring.git</span></span>
<span class="line"><span>git clone https://github.com/hpcc-systems/ganglia-monitoring.git</span></span></code></pre></div><p>The files git-fixversion and git-unupmerge can copied so they are on your default path, and then they will be available as git commands.</p><h2 id="tagging-new-versions-1" tabindex="-1">Tagging new versions <a class="header-anchor" href="#tagging-new-versions-1" aria-label="Permalink to &quot;Tagging new versions&quot;">​</a></h2><p>The following process should be followed when tagging a new set of versions.</p><ol><li>Upmerge all changes between candidate branches for the different versions</li></ol><p>You can set the <code>all</code> environment variable to a subset of the projects (e.g. <code>export all=hpcc</code>) if there are no changes in the other repositories. The only effect for projects that are upmerged with no changes will be that they gain an empty merge transaction. If multiple people are merging PRs to different repositories it may be safer to upmerge all projects.</p><p>For example:</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>./upmerge A.a.x candidate-A.b.x</span></span>
<span class="line"><span>./upmerge A.b.x candidate-A.c.x</span></span>
<span class="line"><span>./upmerge A.b.x candidate-B.0.x</span></span>
<span class="line"><span>./upmerge B.0.x master</span></span></code></pre></div><ol start="2"><li>Create new point-release candidate branches:</li></ol><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>./gorc.sh A.a.x</span></span>
<span class="line"><span>./gorc.sh A.b.x</span></span>
<span class="line"><span>./gorc.sh A.c.x</span></span></code></pre></div><h2 id="taking-a-build-gold" tabindex="-1">Taking a build gold: <a class="header-anchor" href="#taking-a-build-gold" aria-label="Permalink to &quot;Taking a build gold:&quot;">​</a></h2><p>Go gold with each of the explicit versions</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>./gogold.sh 7.8.76</span></span>
<span class="line"><span>./gogold.sh 7.10.50</span></span></code></pre></div><p>If you have merged changes onto a point-release branch you would normally create a new rc before going gold. If the change was trivial (e.g. removing an unwanted file) then you can use the --ignore option to skip that step.</p><h2 id="creating-a-new-rc-for-an-existing-point-release" tabindex="-1">Creating a new rc for an existing point release: <a class="header-anchor" href="#creating-a-new-rc-for-an-existing-point-release" aria-label="Permalink to &quot;Creating a new rc for an existing point release:&quot;">​</a></h2><p>This normally happens after cherry-picking a late fix for a particular version, which has already been merged into the .x candidate branch.</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>./gorc.sh A.a.&lt;n&gt;</span></span></code></pre></div><h2 id="create-a-new-minor-major-branch" tabindex="-1">Create a new minor/major branch <a class="header-anchor" href="#create-a-new-minor-major-branch" aria-label="Permalink to &quot;Create a new minor/major branch&quot;">​</a></h2><p>A new minor branch is created from the current master...</p><div class="language- vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span>./gominor.sh</span></span></code></pre></div>`,30),o=[i];function p(l,r,c,h,g,d){return s(),a("div",null,o)}const b=e(t,[["render",p]]);export{m as __pageData,b as default};
