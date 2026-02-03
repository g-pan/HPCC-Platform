# Documenting Certificate Use for Remote DFS Clients in HPCC Systems (Cloud/Kubernetes)

This guide details how to configure cloud HPCC environments and use client certificates for remote DFS (Distributed File System) access between clusters, using Helm and Kubernetes best practices.

## Contents

- [Overview](#overview)
- [Best Practices: YAML and Helm Values](#best-practices-yaml-and-helm-values)
- [Step-by-Step Setup](#step-by-step-setup)
  - [Server Side (env1) Setup](#server-side-env1-setup)
  - [Client Side (env2) Setup](#client-side-env2-setup)
- [Testing](#testing)
- [Key Takeaways](#key-takeaways)
- [References](#references)

---

## Overview

This example configures two HPCC Helm environments (env1 and env2), so that env2 (client) can securely access logical files from env1 (server) via DFS, with client certificates for mTLS.

Environments are assumed to be in separate Kubernetes namespaces for simplicity, but could be on distinct clusters.

---

## Best Practices: YAML and Helm Values

- **Isolated Custom YAMLs:** Separate changes into custom YAML files (per area/component). For example, keep certificate-related customizations apart from unrelated settings.
- **Merging Values:** Helm will merge the base chart defaults with any provided custom values YAMLs at install/update.
  
---

## Step-by-Step Setup

### Server Side (env1) Setup

1. **Prepare Namespace**
   ```shell
   kubectl create ns env1
   kubectl config set-context --current --namespace=env1
   ```

2. **Install Jetstack cert-manager**  
   Needed for automatic certificate management.
   ```shell
   helm repo add jetstack https://charts.jetstack.io
   helm repo update
   helm install cert-manager jetstack/cert-manager --version v1.7.1 --set installCRDs=true
   ```

3. **Enable Certificates in Custom Values YAML**
   ```yaml
   certificates:
     enabled: true
   ```

4. **Configure Remote Certificate Issuer**
   ```yaml
   certificates:
     issuers:
       remote:
         name: hpcc-remote-issuer
         enabled: true
         kind: Issuer
         spec:
           ca:
             secretName: hpcc-remote-issuer-key-pair
   ```
   > **Note:** To enable certificates only for ESP/DFS, set `enabled: false` under other issuers (`local`, `public`, `signing`).

5. **Configure ESP DFS Service with Remote Clients**  
   In your ESP service (e.g., within a YAML chunk):
   ```yaml
   - name: dfs
     application: dfs
     auth: none
     replicas: 1
     remoteClients:
     - name: env2
       organization: mycompany
     service:
       visibility: local
       servicePort: 8520
   ```

6. **Create Root Certificate Authority and Secret**
   ```shell
   openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -sha256 -days 1825 -out ca.crt \
     -subj "/C=US/ST=Georgia/L=Alpharetta/O=HPCC Systems/OU=HPCC Example/CN=info@hpccsystems.com"

   kubectl create secret tls hpcc-remote-issuer-key-pair --cert=ca.crt --key=ca.key
   ```

7. **Deploy HPCC with Custom Settings**
   ```shell
   helm install env1 hpcc/hpcc -f mycustomsettings.yaml
   ```

8. **Extract Client Certificate and Key**
   List secrets:
   ```shell
   kubectl get secrets
   ```
   Extract secrets for the client (typically prefixed `client-remote-dfs-`):
   ```shell
   kubectl get secret client-remote-dfs-dfs-env2-tls -o json | jq -r '.data."tls.crt"' | base64 -d > env2tls.crt
   kubectl get secret client-remote-dfs-dfs-env2-tls -o json | jq -r '.data."tls.key"' | base64 -d > env2tls.key
   kubectl get secret client-remote-dfs-dfs-env2-tls -o json | jq -r '.data."ca.crt"' | base64 -d > env2ca.crt
   ```

9. **Record the DFS Service Endpoint**
   ```shell
   kubectl get svc dfs
   ```
   Example output:
   ```
   NAME   TYPE         CLUSTER-IP     EXTERNAL-IP    PORT(S)
   dfs    LoadBalancer 10.0.108.249   192.168.0.36   8520:XXXXX/TCP
   ```
   Note down the `EXTERNAL-IP` and relevant port.

---

### Client Side (env2) Setup

1. **Switch/Create Namespace**
   ```shell
   kubectl create ns env2
   kubectl config set-context --current --namespace=env2
   ```

2. **Create Secret from Exported Certs**
   ```shell
   kubectl create secret generic env1-access \
     --from-file=env2tls.crt --from-file=env2tls.key --from-file=env2ca.crt
   ```

3. **Reference Secret in Custom Values YAML**
   ```yaml
   secrets:
     storage:
       env1-access: env1-access
   ```

4. **Reference Remote Data Plane (Storage and Remotes)**
   - Ensure a PVC (Persistent Volume Claim) exists, mapping to the remote storage — here called `env1-pvc`.
   - In your custom values YAML:
   ```yaml
   storage:
     planes:
     - name: env1-data
       pvc: env1-pvc
       prefix: "/var/lib/HPCCSystems/env1/hpcc-data"
       category: remote
     remote:
     - name: env1
       service: https://192.168.0.36:8520
       secret: env1-access
       planes:
       - remote: data
         local: env1-data
   ```
   > ⚠️ *Do not use this YAML chunk standalone if you already have plane definitions—merge carefully to avoid replacing existing lists.*

5. **Deploy Client HPCC Environment**
   ```shell
   helm install env2 hpcc/hpcc -f client-remotesetup.yaml
   ```

---

## Testing

1. **Create a test logical file on Server (env1) via ECL:**
   ```ecl
   ds := DATASET([{1, 'one'}, {2, 'two'}, {3, 'three'}], {unsigned num, string str}, DISTRIBUTED);
   OUTPUT(ds, , '~somescope::somefilename', OVERWRITE);
   ```
   Run via ECL command line or EclWatch playground.

2. **Read from Client (env2) via ECL:**
   ```ecl
   ds := DATASET('~remote::env1::somescope::somefilename', {unsigned num, string str}, FLAT);
   OUTPUT(ds, ALL);
   ```
   Run via ECL command line or EclWatch playground.

---

## Key Takeaways

- **Best Practice:** Isolate configuration changes per area in separate YAML files and merge via Helm.
- **Certificate Management:** Use cert-manager to automate CA and client certificate creation in K8s.
- **RemoteClients:** Define remote clients in ESP settings to trigger remote client certificate generation.
- **Secrets & Endpoints:** Export and handle secrets securely; record remote DFS service endpoints for client configuration.
- **PVC and Planes:** Ensure correct mapping of storage volumes and logical planes across clusters.
- **Security:** mTLS ensures encrypted/authenticated cross-cluster communication.

---

## References

- [HPCC Helm Charts](https://github.com/hpcc-systems/helm-chart)
- [Cert-manager Documentation](https://cert-manager.io/docs/)
- [Kubernetes Secrets](https://kubernetes.io/docs/concepts/configuration/secret/)
- [Helm Custom Values](https://helm.sh/docs/chart_template_guide/values_files/)
- [HPCC Systems](https://hpccsystems.com/)