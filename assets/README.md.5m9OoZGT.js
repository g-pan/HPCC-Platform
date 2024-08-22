import{_ as e,c as t,o as a,V as i}from"./chunks/framework.gBlNPWt_.js";const g=JSON.parse('{"title":"Description / Rationale","description":"","frontmatter":{},"headers":[],"relativePath":"README.md","filePath":"README.md","lastUpdated":1724329493000}'),r={name:"README.md"},s=i(`<h1 id="description-rationale" tabindex="-1">Description / Rationale <a class="header-anchor" href="#description-rationale" aria-label="Permalink to &quot;Description / Rationale&quot;">​</a></h1><p>HPCC Systems offers an enterprise ready, open source supercomputing platform to solve big data problems. As compared to Hadoop, the platform offers analysis of big data using less code and less nodes for greater efficiencies and offers a single programming language, a single platform and a single architecture for efficient processing. HPCC Systems is a technology division of LexisNexis Risk Solutions.</p><h1 id="getting-started" tabindex="-1">Getting Started <a class="header-anchor" href="#getting-started" aria-label="Permalink to &quot;Getting Started&quot;">​</a></h1><ul><li><a href="https://hpccsystems.com/about#Platform" target="_blank" rel="noreferrer">Learn about HPCC</a></li><li><a href="https://hpccsystems.com/download" target="_blank" rel="noreferrer">Download</a></li><li><a href="https://hpccsystems.com/training/documentation/installation-and-administration" target="_blank" rel="noreferrer">Installation and Running</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/" target="_blank" rel="noreferrer">Developer Documentation</a></li></ul><h1 id="architecture" tabindex="-1">Architecture <a class="header-anchor" href="#architecture" aria-label="Permalink to &quot;Architecture&quot;">​</a></h1><p>The HPCC Systems architecture incorporates the Thor and Roxie clusters as well as common middleware components, an external communications layer, client interfaces which provide both end-user services and system management tools, and auxiliary components to support monitoring and to facilitate loading and storing of filesystem data from external sources. An HPCC environment can include only Thor clusters, or both Thor and Roxie clusters. Each of these cluster types is described in more detail in the following sections below the architecture diagram.</p><h2 id="thor" tabindex="-1">Thor <a class="header-anchor" href="#thor" aria-label="Permalink to &quot;Thor&quot;">​</a></h2><p>Thor (the Data Refinery Cluster) is responsible for consuming vast amounts of data, transforming, linking and indexing that data. It functions as a distributed file system with parallel processing power spread across the nodes. A cluster can scale from a single node to thousands of nodes.</p><ul><li>Single-threaded</li><li>Distributed parallel processing</li><li>Distributed file system</li><li>Powerful parallel processing programming language (ECL)</li><li>Optimized for Extraction, Transformation, Loading, Sorting, Indexing and Linking</li><li>Scales from 1-1000s of nodes</li></ul><h2 id="roxie" tabindex="-1">Roxie <a class="header-anchor" href="#roxie" aria-label="Permalink to &quot;Roxie&quot;">​</a></h2><p>Roxie (the Query Cluster) provides separate high-performance online query processing and data warehouse capabilities. Roxie (Rapid Online XML Inquiry Engine) is the data delivery engine used in HPCC to serve data quickly and can support many thousands of requests per node per second.</p><ul><li>Multi-threaded</li><li>Distributed parallel processing</li><li>Distributed file system</li><li>Powerful parallel processing programming language (ECL)</li><li>Optimized for concurrent query processing</li><li>Scales from 1-1000s of nodes</li></ul><h2 id="ecl" tabindex="-1">ECL <a class="header-anchor" href="#ecl" aria-label="Permalink to &quot;ECL&quot;">​</a></h2><p>ECL (Enterprise Control Language) is the powerful programming language that is ideally suited for the manipulation of Big Data.</p><ul><li>Transparent and implicitly parallel programming language</li><li>Non-procedural and dataflow oriented</li><li>Modular, reusable, extensible syntax</li><li>Combines data representation and algorithm implementation</li><li>Easily extend using C++ libraries</li><li>ECL is compiled into optimized C++</li></ul><h2 id="ecl-ide" tabindex="-1">ECL IDE <a class="header-anchor" href="#ecl-ide" aria-label="Permalink to &quot;ECL IDE&quot;">​</a></h2><p>ECL IDE is a modern IDE used to code, debug and monitor ECL programs.</p><ul><li>Access to shared source code repositories</li><li>Complete development, debugging and testing environment for developing ECL dataflow programs</li><li>Access to the ECLWatch tool is built-in, allowing developers to watch job graphs as they are executing</li><li>Access to current and historical job workunits</li></ul><h2 id="esp" tabindex="-1">ESP <a class="header-anchor" href="#esp" aria-label="Permalink to &quot;ESP&quot;">​</a></h2><p>ESP (Enterprise Services Platform) provides an easy to use interface to access ECL queries using XML, HTTP, SOAP and REST.</p><ul><li>Standards-based interface to access ECL functions</li></ul><h1 id="developer-documentation" tabindex="-1">Developer documentation <a class="header-anchor" href="#developer-documentation" aria-label="Permalink to &quot;Developer documentation&quot;">​</a></h1><p>The following links describe the structure of the system and detail some of the key components:</p><ul><li><a href="https://hpcc-systems.github.io/HPCC-Platform/devdoc/Workunits.html" target="_blank" rel="noreferrer">An overview of workunits and the different stages in executing a query</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/devdoc/CodeGenerator.html" target="_blank" rel="noreferrer">An introduction to the code generator - eclcc</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/devdoc/MemoryManager.html" target="_blank" rel="noreferrer">The memory manager used by roxie and thor</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/initfiles/DOCUMENTATION.html" target="_blank" rel="noreferrer">The structure of the initialization scripts</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/ecl/ecl-bundle/DOCUMENTATION.html" target="_blank" rel="noreferrer">Outline of ecl-bundle</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/cmake_modules/DOCUMENTATION.html" target="_blank" rel="noreferrer">The structure and some details of the cmake files</a></li><li><a href="https://hpcc-systems.github.io/HPCC-Platform/docs/DOCUMENTATION.html" target="_blank" rel="noreferrer">Building the documentation</a></li></ul><h2 id="regression-test" tabindex="-1">Regression test <a class="header-anchor" href="#regression-test" aria-label="Permalink to &quot;Regression test&quot;">​</a></h2><div class="language-sh vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">sh</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;">cd</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> /opt/HPCCSystems/testing/regress</span></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./ecl-test</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> query --target thor nlppp.ecl</span></span></code></pre></div>`,26),o=[s];function n(l,c,d,h,p,u){return a(),t("div",null,o)}const f=e(r,[["render",n]]);export{g as __pageData,f as default};
