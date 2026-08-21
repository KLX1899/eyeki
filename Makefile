PREFIX ?= /usr
DESTDIR ?=

BINDIR ?= $(PREFIX)/bin
SYSTEMD_USER_DIR ?= $(PREFIX)/lib/systemd/user

CC ?= gcc
PKG_CONFIG ?= pkg-config

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 libnotify libsystemd)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 libnotify libsystemd)

override CFLAGS += -Wall -Wextra $(PKG_CFLAGS)

TARGET = eyeki
SOURCES = eyeki.c config.c config_watch.c runtime.c scheduler.c
HEADERS = config.h config_watch.h runtime.h scheduler.h version.h
SERVICE = eyeki.service

TEST_TARGETS = tests/test_scheduler tests/test_config tests/test_config_watch \
	tests/test_runtime

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(PKG_LIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 $(SERVICE) $(DESTDIR)$(SYSTEMD_USER_DIR)/$(SERVICE)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(SYSTEMD_USER_DIR)/$(SERVICE)

clean:
	rm -f $(TARGET) $(TEST_TARGETS)

test: $(TEST_TARGETS)
	./tests/test_scheduler
	./tests/test_config
	./tests/test_config_watch
	./tests/test_runtime

tests/test_scheduler: tests/test_scheduler.c scheduler.c scheduler.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_scheduler.c scheduler.c

tests/test_config: tests/test_config.c config.c config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_config.c config.c

tests/test_config_watch: tests/test_config_watch.c config.c config.h \
		config_watch.c config_watch.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_config_watch.c \
		config.c config_watch.c

tests/test_runtime: tests/test_runtime.c runtime.c runtime.h config.c config.h \
		scheduler.c scheduler.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_runtime.c runtime.c \
		config.c scheduler.c

.PHONY: all install uninstall clean test
