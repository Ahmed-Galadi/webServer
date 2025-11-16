# ============ Makefile ============
NAME = webserv
CXX = c++
CXXFLAGS = -std=c++98 -g -MMD -Wall -Wextra -Werror

SRCDIR = src
INCDIR = include
OBJDIR = obj

SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/server/Server.cpp \
          $(SRCDIR)/server/EventManager.cpp \
          $(SRCDIR)/config/ServerConfig.cpp \
          $(SRCDIR)/config/LocationConfig.cpp \
          $(SRCDIR)/config/ParseUtils.cpp \
          $(SRCDIR)/config/ParsingBlock.cpp \
          $(SRCDIR)/config/ConfigParser.cpp \
          $(SRCDIR)/client/Client.cpp \
          $(SRCDIR)/http/requestParse/Request.cpp \
		  $(SRCDIR)/http/requestParse/RequestBody.cpp \
		  $(SRCDIR)/http/requestParse/RequestParser.cpp \
          $(SRCDIR)/http/response/Response.cpp \
		  $(SRCDIR)/http/response/HttpMethodhandler.cpp \
		  $(SRCDIR)/http/httpMethods/post/POSThandle_form.cpp \
		  $(SRCDIR)/http/httpMethods/post/POSThandle_json.cpp \
		  $(SRCDIR)/http/httpMethods/post/POSThandle_multipart.cpp \
		  $(SRCDIR)/http/httpMethods/post/POSThandle_others.cpp \
		  $(SRCDIR)/http/httpMethods/post/POSThandler.cpp \
		  $(SRCDIR)/http/httpMethods/utils/MimeType.cpp \
		  $(SRCDIR)/http/httpMethods/utils/FileHandler.cpp \
		  $(SRCDIR)/utils/GlobalUtils.cpp \
		  $(SRCDIR)/http/httpMethods/get/GEThandler.cpp \
		  $(SRCDIR)/http/httpMethods/delete/DELETEhandler.cpp \
		  $(SRCDIR)/http/httpMethods/cgi/CGIhandler.cpp \


OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPFILES = $(OBJECTS:.o=.d)

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Development targets
test: $(NAME)
	./$(NAME) config/default.conf

debug: CXXFLAGS += -DDEBUG
debug: re

# Include dependency files if they exist
-include $(DEPFILES)
