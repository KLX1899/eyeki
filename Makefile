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
SOURCES = eyeki.c config.c scheduler.c
HEADERS = config.h scheduler.h version.h
SERVICE = eyeki.service

TEST_TARGET = tests/test_scheduler
TEST_SOURCES = tests/test_scheduler.c scheduler.c

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
	rm -f $(TARGET) $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) scheduler.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(TEST_SOURCES)

.PHONY: all install uninstall clean test
