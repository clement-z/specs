# Version header for `specs --version` and the run manifest (src/main.cpp and src/run_manifest.cpp
# include <utils/version.h>; run_manifest.cpp is a core source, so libspx.so needs version_dep too),
# computed from the git tags like the former cmake/version.cmake. `full` is refreshed at every build
# (vcs_tag) and falls back to the project version outside a git checkout; major/minor/patch are set at
# configure time.
git_version_sh = find_program(meson.project_source_root() / 'utils/git-version.sh')
version_conf = configuration_data()
foreach part : ['major', 'minor', 'patch']
  r = run_command(git_version_sh, meson.project_source_root(), part, check: false)
  version_conf.set('VERSION_' + part.to_upper(), r.returncode() == 0 ? r.stdout().strip() : '0')
endforeach
version_h = vcs_tag(
  command: [git_version_sh, meson.project_source_root(), 'full'],
  fallback: meson.project_version(),
  input: configure_file(input: 'version.h.in', output: 'version.h.in', configuration: version_conf),
  output: 'version.h',
  replace_string: '%VERSION_FULL%',
)
# <utils/version.h> resolves against <builddir>/src.
version_dep = declare_dependency(sources: version_h,
                                 compile_args: ['-I' + meson.project_build_root() / 'src'])
