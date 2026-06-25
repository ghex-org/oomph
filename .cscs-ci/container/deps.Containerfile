ARG BASE_IMAGE
FROM $BASE_IMAGE

ARG SPACK_SHA
RUN mkdir -p /opt/spack && \
    curl -fLsS "https://api.github.com/repos/spack/spack/tarball/$SPACK_SHA" | tar --strip-components=1 -xz -C /opt/spack

ENV PATH="/opt/spack/bin:$PATH"

ARG SPACK_PACKAGES_SHA
RUN mkdir -p /opt/spack-packages && \
    curl -fLsS "https://api.github.com/repos/spack/spack-packages/tarball/$SPACK_PACKAGES_SHA" | tar --strip-components=1 -xz -C /opt/spack-packages

RUN spack repo remove --scope defaults:base builtin && \
    spack repo add --scope site /opt/spack-packages/repos/spack_repo/builtin


RUN cat <<EOF > /opt/spack-packages/repos/spack_repo/builtin/packages/libfabric/mr_unsubscribe.patch
diff --git a/prov/util/src/import_mem_monitor.c b/prov/util/src/import_mem_monitor.c
index e7be581526f0..1f1f4a971099 100644
--- a/prov/util/src/import_mem_monitor.c
+++ b/prov/util/src/import_mem_monitor.c
@@ -111,7 +111,7 @@ static void ofi_import_monitor_unsubscribe(struct ofi_mem_monitor *notifier,
                       const void *addr, size_t len,
                       union ofi_mr_hmem_info *hmem_info)
 {
-	assert(impmon.impfid);
+	if (!impmon.impfid) return;
    impmon.impfid->export_ops->unsubscribe(impmon.impfid, addr, len);
 }
EOF

RUN sed -i '/patch("nvhpc-symver.patch", when="@1.6.0:1.14.0 %nvhpc")/a\    patch("mr_unsubscribe.patch")' /opt/spack-packages/repos/spack_repo/builtin/packages/libfabric/package.py


ARG SPACK_ENV_FILE
COPY $SPACK_ENV_FILE /spack_environment/spack.yaml

ARG NUM_PROCS
RUN spack external find --all && \
    spack env create ci /spack_environment/spack.yaml && \
    spack -e ci concretize -f && \
    spack -e ci install --jobs $NUM_PROCS --fail-fast --only=dependencies
