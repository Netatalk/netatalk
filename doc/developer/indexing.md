Indexing for Spotlight Search
=============================

Starting with version 3.1 Netatalk supports Spotlight searching.
Netatalk uses GNOME TinySPARQL/[LocalSearch](https://gnome.pages.gitlab.gnome.org/localsearch/)
(previously known as Tracker) as metadata store, indexer and search engine.

Limitations and notes
---------------------

* Large filesystems

    Tracker on Linux uses the inotify Kernel filesystem change event API
    for tracking filesystem changes. On large filesystems this may be
    problematic since the inotify API doesn't offer recursive directory
    watches but instead requires that for every subdirectory watches must
    be added individually.

    On Solaris the FEN file event notification system is used. It is
    unknown which limitations and resource consumption this Solaris
    subsystem may have.

    We therefore recommend to disable live filesystem monitoring and let
    Tracker periodically scan filesystems for changes instead, see the
    Tracker configuration options enable-monitors and crawling-interval below.

* Indexing home directories

    A known limitation with the current implementation means that shared
    volumes in a user's home directory does not get indexed by Spotlight.

    As a workaround, keep the shared volumes you want to have indexed
    elsewhere on the host filesystem.

Supported metadata attributes
-----------------------------

The following table lists the supported Spotlight metadata attributes,
based on Apple's [MDItem object](https://developer.apple.com/documentation/coreservices/mditemref/)
Core Services file metadata standard.

| Description | Spotlight Key |
|-------------|---------------|
| Name | kMDItemDisplayName, kMDItemFSName |
| Document content (full text search) | kMDItemTextContent |
| File type | \_kMDItemGroupId, kMDItemContentTypeTree |
| File modification date | kMDItemFSContentChangeDate, kMDItemContentModificationDate, kMDItemAttributeChangeDate |
| Content Creation date | kMDItemContentCreationDate |
| The author, or authors, of the contents of the file | kMDItemAuthors, kMDItemCreator |
| The name of the country where the item was created | kMDItemCountry |
| Duration | kMDItemDurationSeconds |
| Number of pages | kMDItemNumberOfPages |
| Document title | kMDItemTitle |
| The width, in pixels, of the contents. For example, the image width or the video frame width | kMDItemPixelWidth |
| The height, in pixels, of the contents. For example, the image height or the video frame height | kMDItemPixelHeight |
| The color space model used by the document contents | kMDItemColorSpace |
| The number of bits per sample | kMDItemBitsPerSample |
| Focal length of the lens, in millimeters | kMDItemFocalLength |
| ISO speed | kMDItemISOSpeed |
| Orientation of the document. Possible values are 0 (landscape) and 1 (portrait) | kMDItemOrientation |
| Resolution width, in DPI | kMDItemResolutionWidthDPI |
| Resolution height, in DPI | kMDItemResolutionHeightDPI |
| Exposure time, in seconds | kMDItemExposureTimeSeconds |
| The composer of the music contained in the audio file | kMDItemComposer |
| The musical genre of the song or composition | kMDItemMusicalGenre |
