# niieux-nautilus

The customized file manager for [Niieux GNU/Linux](https://niieux.com/) (Still under development), based on GNOME Files (Nautilus).

## About

This fork of Nautilus is tailored for Niieux GNU/Linux, providing an optimized file browsing experience that integrates seamlessly with the Niieux desktop environment. For example, it includes changes to the sidebar, like a new System/Root section.

## Runtime Dependencies

- [Bubblewrap](https://github.com/projectatomic/bubblewrap) - Required for sandboxing and security
- [LocalSearch](https://gitlab.gnome.org/GNOME/localsearch) - Required for fast search, metadata extraction, and starred files
- [xdg-user-dirs-gtk](https://gitlab.gnome.org/GNOME/xdg-user-dirs-gtk) - Required for default bookmarks and localization

## Upstream

This project is based on [GNOME Nautilus](https://gitlab.gnome.org/GNOME/nautilus). For upstream documentation and API reference, visit the [official Nautilus documentation](https://gnome.pages.gitlab.gnome.org/nautilus/).
