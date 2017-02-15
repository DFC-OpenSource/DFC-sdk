"use strict"

function layerDetailsPageInit (ctx) {

  var layerDepInput = $("#layer-dep-input");
  var layerDepBtn = $("#add-layer-dependency-btn");
  var layerDepsList = $("#layer-deps-list");
  var currentLayerDepSelection;
  var addRmLayerBtn = $("#add-remove-layer-btn");

  /* setup the dependencies typeahead */
  libtoaster.makeTypeahead(layerDepInput, ctx.xhrDataTypeaheadUrl, { type : "layers", project_id: ctx.projectId, include_added: "true" }, function(item){
    currentLayerDepSelection = item;

    layerDepBtn.removeAttr("disabled");
  });

  function addRemoveDep(depLayerId, add, doneCb) {
    var data = { layer_version_id : ctx.layerVersion.id };
    if (add)
      data.add_dep = depLayerId;
    else
      data.rm_dep = depLayerId;

    $.ajax({
        type: "POST",
        url: ctx.xhrUpdateLayerUrl,
        data: data,
        headers: { 'X-CSRFToken' : $.cookie('csrftoken')},
        success: function (data) {
          if (data.error != "ok") {
            console.warn(data.error);
          } else {
            doneCb();
          }
        },
        error: function (data) {
          console.warn("Call failed");
          console.warn(data);
        }
    });
  }

  function layerRemoveClick() {
    var toRemove = $(this).parent().data('layer-id');
    var layerDepItem = $(this);

    addRemoveDep(toRemove, false, function(){
      layerDepItem.parent().fadeOut(function (){
        layerDepItem.remove();
      });
    });
  }

  /* Add dependency layer button click handler */
  layerDepBtn.click(function(){
    if (currentLayerDepSelection == undefined)
      return;

    addRemoveDep(currentLayerDepSelection.id, true, function(){
      /* Make a list item for the new layer dependency */
      var newLayerDep = $("<li><a></a><span class=\"icon-trash\" data-toggle=\"tooltip\" title=\"Delete\"></span></li>");

      newLayerDep.data('layer-id', currentLayerDepSelection.id);
      newLayerDep.children("span").tooltip();

      var link = newLayerDep.children("a");
      link.attr("href", ctx.layerDetailsUrl+String(currentLayerDepSelection.id));
      link.text(currentLayerDepSelection.name);
      link.tooltip({title: currentLayerDepSelection.tooltip, placement: "right"});

      /* Connect up the tash icon */
      var trashItem = newLayerDep.children("span");
      trashItem.click(layerRemoveClick);

      layerDepsList.append(newLayerDep);
      /* Clear the current selection */
      layerDepInput.val("");
      currentLayerDepSelection = undefined;
      layerDepBtn.attr("disabled","disabled");
    });
  });

  $(".icon-pencil").click(function (){
    var mParent = $(this).parent("dd");
    mParent.prev().css("margin-top", "10px");
    mParent.children("form").slideDown();
    var currentVal = mParent.children(".current-value");
    currentVal.hide();
    /* Set the current value to the input field */
    mParent.find("textarea,input").val(currentVal.text());
    /* Hides the "Not set" text */
    mParent.children(".muted").hide();
    /* We're editing so hide the delete icon */
    mParent.children(".delete-current-value").hide();
    mParent.find(".cancel").show();
    $(this).hide();
  });

  $(".delete-current-value").click(function(){
    var mParent = $(this).parent("dd");
    mParent.find("input").val("");
    mParent.find("textarea").val("");
    mParent.find(".change-btn").click();
  });

  $(".cancel").click(function(){
    var mParent = $(this).parents("dd");
    $(this).hide();
    mParent.children("form").slideUp(function(){
      mParent.children(".current-value").show();
      /* Show the "Not set" text if we ended up with no value */
      if (!mParent.children(".current-value").html()){
        mParent.children(".muted").fadeIn();
        mParent.children(".delete-current-value").hide();
      } else {
        mParent.children(".delete-current-value").show();
      }

      mParent.children(".icon-pencil").show();
      mParent.prev().css("margin-top", "0px");
    });
  });

  $(".build-target-btn").click(function(){
    /* fire a build */
    var target = $(this).data('target-name');
    libtoaster.startABuild(ctx.projectBuildUrl, ctx.projectId, target, null, null);
    window.location.replace(ctx.projectPageUrl);
  });

  $(".select-machine-btn").click(function(){
    var data =  { machineName : $(this).data('machine-name') };
    libtoaster.editProject(ctx.xhrEditProjectUrl, ctx.projectId, data,
      function (){
        window.location.replace(ctx.projectPageUrl+"#/machineselected");
    }, null);
  });

  function defaultAddBtnText(){
      var text = " Add the "+ctx.layerVersion.name+" layer to your project";
      addRmLayerBtn.text(text);
      addRmLayerBtn.prepend("<span class=\"icon-plus\"></span>");
      addRmLayerBtn.removeClass("btn-danger");
  }

  $("#details-tab").on('show', function(){
    if (!ctx.layerVersion.inCurrentPrj)
      defaultAddBtnText();

    window.location.hash = "details";
  });

  function targetsTabShow(){
    if (!ctx.layerVersion.inCurrentPrj){
      if (ctx.numTargets > 0) {
        var text = " Add the "+ctx.layerVersion.name+" layer to your project "+
          "to enable these recipes";
        addRmLayerBtn.text(text);
        addRmLayerBtn.prepend("<span class=\"icon-plus\"></span>");
      } else {
        defaultAddBtnText();
      }
    }

    window.location.hash = "targets";
  }

  $("#targets-tab").on('show', targetsTabShow);

  function machinesTabShow(){
    if (!ctx.layerVersion.inCurrentPrj) {
      if (ctx.numMachines > 0){
        var text = " Add the "+ctx.layerVersion.name+" layer to your project " +
          "to enable these machines";
        addRmLayerBtn.text(text);
        addRmLayerBtn.prepend("<span class=\"icon-plus\"></span>");
      } else {
        defaultAddBtnText();
      }
    }

    window.location.hash = "machines";
  }

  $("#machines-tab").on('show', machinesTabShow);

  $(".pagesize").change(function(){
    var search = libtoaster.parseUrlParams();
    search.limit = this.value;

    window.location.search = libtoaster.dumpsUrlParams(search);
  });

  /* Enables the Build target and Select Machine buttons and switches the
   * add/remove button
   */
  function setLayerInCurrentPrj(added, depsList) {
    ctx.layerVersion.inCurrentPrj = added;
    var alertMsg = $("#alert-msg");
    /* Reset alert message */
    alertMsg.text("");

    if (added){
      /* enable and switch all the button states */
      $(".build-target-btn").removeAttr("disabled");
      $(".select-machine-btn").removeAttr("disabled");
      addRmLayerBtn.addClass("btn-danger");
      addRmLayerBtn.data('directive', "remove");
      addRmLayerBtn.text(" Delete the "+ctx.layerVersion.name+" layer from your project");
      addRmLayerBtn.prepend("<span class=\"icon-trash\"></span>");

      if (depsList) {
        alertMsg.append("You have added <strong>"+(depsList.length+1)+"</strong> layers to <a id=\"project-affected-name\"></a>: <span id=\"layer-affected-name\"></span> and its dependencies ");

        /* Build the layer deps list */
        depsList.map(function(layer, i){
          var link = $("<a></a>");

          link.attr("href", layer.layerdetailurl);
          link.text(layer.name);
          link.tooltip({title: layer.tooltip});

          if (i != 0)
            alertMsg.append(", ");

          alertMsg.append(link);
        });
      } else {
        alertMsg.append("You have added <strong>1</strong> layer to <a id=\"project-affected-name\"></a>: <span id=\"layer-affected-name\"></span>");
      }
    } else {
      /* disable and switch all the button states */
      $(".build-target-btn").attr("disabled","disabled");
      $(".select-machine-btn").attr("disabled", "disabled");
      addRmLayerBtn.removeClass("btn-danger");
      addRmLayerBtn.data('directive', "add");

      /* "special" handler so that we get the correct button text which depends
       * on which tab is currently visible. Unfortunately we can't just call
       * tab('show') as if it's already visible it doesn't run the event.
       */
      switch ($(".nav-pills .active a").prop('id')){
        case 'machines-tab':
          machinesTabShow();
          break;
        case 'targets-tab':
          targetsTabShow();
          break;
        default:
          defaultAddBtnText();
          break;
      }

      alertMsg.append("You have deleted <strong>1</strong> layer from <a id=\"project-affected-name\"></a>: <strong id=\"layer-affected-name\"></strong>");
    }

    alertMsg.children("#layer-affected-name").text(ctx.layerVersion.name);
    alertMsg.children("#project-affected-name").text(ctx.projectName);
    alertMsg.children("#project-affected-name").attr("href", ctx.projectPageUrl);
    $("#alert-area").show();
  }

  $("#dismiss-alert").click(function(){ $(this).parent().hide() });

  /* Add or remove this layer from the project */
  addRmLayerBtn.click(function() {
    var directive = $(this).data('directive');

    if (directive == 'add') {
      /* If adding get the deps for this layer */
      libtoaster.getLayerDepsForProject(ctx.xhrDataTypeaheadUrl, ctx.projectId, ctx.layerVersion.id, function (data) {
        /* got result for dependencies */
        if (data.list.length == 0){
          var editData = { layerAdd : ctx.layerVersion.id };
          libtoaster.editProject(ctx.xhrEditProjectUrl, ctx.projectId, editData,
            function() {
              setLayerInCurrentPrj(true);
          });
          return;
        } else {
          /* The add deps will include this layer so no need to add it
           * separately.
           */
          show_layer_deps_modal(ctx.projectId, ctx.layerVersion, data.list, null, null, true, function () {
            /* Success add deps and layer */
            setLayerInCurrentPrj(true, data.list);
          });
        }
      }, null);
    } else if (directive == 'remove') {
      var editData = { layerDel : ctx.layerVersion.id };

      libtoaster.editProject(ctx.xhrEditProjectUrl, ctx.projectId, editData,
        function () {
          /* Success removed layer */
           //window.location.reload();
           setLayerInCurrentPrj(false);
        }, function () {
          console.warn ("Removing layer from project failed");
      });
    }
  });

  /* Handler for all of the Change buttons */
  $(".change-btn").click(function(){
    var mParent = $(this).parent();
    var prop = $(this).data('layer-prop');

    /* We have inputs, select and textareas to potentially grab the value
     * from.
     */
    var entryElement = mParent.find("input");
    if (entryElement.length == 0)
      entryElement = mParent.find("textarea");
    if (entryElement.length == 0) {
      console.warn("Could not find element to get data from for this change");
      return;
    }

    var data = { layer_version_id: ctx.layerVersion.id };
    data[prop] = entryElement.val();

    $.ajax({
        type: "POST",
        url: ctx.xhrUpdateLayerUrl,
        data: data,
        headers: { 'X-CSRFToken' : $.cookie('csrftoken')},
        success: function (data) {
          if (data.error != "ok") {
            console.warn(data.error);
          } else {
            /* success layer property changed */
            var inputArea = mParent.parents("dd");
            var text;

            text = entryElement.val();

            /* Hide the "Not set" text if it's visible */
            inputArea.find(".muted").hide();
            inputArea.find(".current-value").text(text);
            /* Same behaviour as cancel in that we hide the form/show current
             * value.
             */
            inputArea.find(".cancel").click();
          }
        },
        error: function (data) {
          console.warn("Call failed");
          console.warn(data);
        }
    });
  });

  /* Disable the change button when we have no data in the input */
  $("dl input, dl textarea").on("input",function() {
    if ($(this).val().length == 0)
      $(this).parent().children(".change-btn").attr("disabled", "disabled");
    else
      $(this).parent().children(".change-btn").removeAttr("disabled");
  });

  /* This checks to see if the dt's dd has data in it or if the change data
   * form is visible, otherwise hide it
   */
  $("dl").children().each(function (){
    if ($(this).is("dt")) {
      var dd = $(this).next("dd");
      if (!dd.children("form:visible")|| !dd.find(".current-value").html()){
        if (ctx.layerVersion.sourceId == 3){
        /* There's no current value and the layer is editable
         * so show the "Not set" and hide the delete icon
         */
        dd.find(".muted").show();
        dd.find(".delete-current-value").hide();
        } else {
          /* We're not viewing an editable layer so hide the empty dd/dl pair */
          $(this).hide();
          dd.hide();
        }
      }
    }
  });

  /* Hide the right column if it contains no information */
  if ($("dl.item-info").children(':visible').length == 0) {
    $("dl.item-info").parent().hide();
  }

  /* Clear the current search selection and reload the results */
  $(".target-search-clear").click(function(){
    $("#target-search").val("");
    $(this).parents("form").submit();
  });

  $(".machine-search-clear").click(function(){
    $("#machine-search").val("");
    $(this).parents("form").submit();
  });


  layerDepsList.find(".icon-trash").click(layerRemoveClick);
  layerDepsList.find("a").tooltip();
  $(".icon-trash").tooltip();
  $(".commit").tooltip();

}
